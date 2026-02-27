import React, { useMemo, useRef, useState, useEffect } from "react";

function fmtTime(ts) {
  if (!ts) return new Date().toLocaleTimeString();
  const d = new Date(Number(ts) * 1000);
  return d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
}

export default function App() {
  const WS_URL = useMemo(() => `ws://${window.location.hostname}:8080`, []);

  const [connected, setConnected] = useState(false);

  const [nick, setNick] = useState("");
  const [nickSet, setNickSet] = useState(false);

  const [room, setRoom] = useState("lobby");
  const [roomJoined, setRoomJoined] = useState(false);

  const [pmTo, setPmTo] = useState("");
  const [pmMsg, setPmMsg] = useState("");

  const [users, setUsers] = useState([]);

  const [chat, setChat] = useState([
    {
      type: "system",
      room: "lobby",
      text: "Peerlink is ready. Flow: Connect → Set Nickname → Join Room (lobby/custom).",
      ts: Math.floor(Date.now() / 1000),
    },
  ]);

  const [term, setTerm] = useState([
    { level: "info", text: `Peerlink UI ready. WS: ${WS_URL}`, ts: Math.floor(Date.now() / 1000) },
  ]);

  const wsRef = useRef(null);
  const reconnectRef = useRef({ tries: 0, timer: null });
  const pingRef = useRef({ timer: null });

  const scrollChatRef = useRef(null);
  const scrollTermRef = useRef(null);
  const msgInputRef = useRef(null);

  useEffect(() => {
    if (scrollChatRef.current) scrollChatRef.current.scrollTop = scrollChatRef.current.scrollHeight;
  }, [chat]);

  useEffect(() => {
    if (scrollTermRef.current) scrollTermRef.current.scrollTop = scrollTermRef.current.scrollHeight;
  }, [term]);

  const pushTerm = (text, level = "info") => {
    const line = { level, text, ts: Math.floor(Date.now() / 1000) };
    setTerm((p) => [...p, line]);
    // eslint-disable-next-line no-console
    console.log(`[${level.toUpperCase()}] ${fmtTime(line.ts)} ${text}`);
  };

  const pushChat = (obj) => setChat((p) => [...p, obj]);

  const cleanupTimers = () => {
    if (reconnectRef.current.timer) clearTimeout(reconnectRef.current.timer);
    reconnectRef.current.timer = null;

    if (pingRef.current.timer) clearInterval(pingRef.current.timer);
    pingRef.current.timer = null;
  };

  const safeSend = (obj) => {
    const ws = wsRef.current;
    if (!ws || ws.readyState !== WebSocket.OPEN) {
      pushTerm("Send failed: not connected", "err");
      pushChat({
        type: "error",
        room: roomJoined ? room : "lobby",
        text: "Not connected. Click Connect.",
        ts: Math.floor(Date.now() / 1000),
      });
      return false;
    }
    const raw = JSON.stringify(obj);
    ws.send(raw);
    pushTerm(`TX ${raw}`, "tx");
    return true;
  };

  const startPing = () => {
    if (pingRef.current.timer) clearInterval(pingRef.current.timer);
    pingRef.current.timer = setInterval(() => {
      if (!connected) return;
      safeSend({ type: "ping" });
      pushTerm("Heartbeat ping", "warn");
    }, 15000);
  };

  const scheduleReconnect = () => {
    const r = reconnectRef.current;
    r.tries += 1;
    const delay = Math.min(8000, 500 * Math.pow(2, Math.min(4, r.tries - 1)));
    pushTerm(`Reconnect in ${delay}ms (try #${r.tries})`, "warn");
    if (r.timer) clearTimeout(r.timer);
    r.timer = setTimeout(() => connect(true), delay);
  };

  const connect = (isAuto = false) => {
    const existing = wsRef.current;
    if (existing && (existing.readyState === WebSocket.OPEN || existing.readyState === WebSocket.CONNECTING)) {
      pushTerm("Connect ignored: already connected/connecting", "warn");
      return;
    }

    pushTerm(`${isAuto ? "Auto" : "Manual"} connect → ${WS_URL}`, "info");
    pushChat({ type: "system", room: "lobby", text: `Connecting to ${WS_URL}...`, ts: Math.floor(Date.now() / 1000) });

    const ws = new WebSocket(WS_URL);
    wsRef.current = ws;

    ws.onopen = () => {
      setConnected(true);
      reconnectRef.current.tries = 0;
      pushTerm("WS OPEN", "info");
      pushChat({ type: "system", room: "lobby", text: "Connected ✅ (Now set nickname)", ts: Math.floor(Date.now() / 1000) });
      startPing();
    };

    ws.onmessage = (e) => {
      const raw = String(e.data ?? "");
      pushTerm(`RX ${raw}`, "rx");

      let obj = null;
      try {
        obj = JSON.parse(raw);
      } catch {
        pushChat({
          type: "error",
          room: roomJoined ? room : "lobby",
          text: `Non-JSON from server: ${raw}`,
          ts: Math.floor(Date.now() / 1000),
        });
        return;
      }

      if (obj?.type === "users" && Array.isArray(obj.users)) {
        setUsers(obj.users);
        return;
      }

      if (obj?.type === "pong") {
        pushTerm("Heartbeat pong", "info");
        return;
      }

      pushChat(obj);
    };

    ws.onerror = () => {
      pushTerm("WS ERROR", "err");
      pushChat({ type: "error", room: roomJoined ? room : "lobby", text: "WebSocket error. Check server + URL.", ts: Math.floor(Date.now() / 1000) });
    };

    ws.onclose = () => {
      setConnected(false);
      cleanupTimers();

      setNickSet(false);
      setRoomJoined(false);
      setUsers([]);

      pushTerm("WS CLOSED", "warn");
      pushChat({ type: "system", room: "lobby", text: "Disconnected ❌", ts: Math.floor(Date.now() / 1000) });
      scheduleReconnect();
    };
  };

  const disconnect = () => {
    pushTerm("Manual disconnect", "warn");
    cleanupTimers();
    if (wsRef.current) wsRef.current.close();
  };

  const onSetNick = () => {
    if (!connected) {
      pushChat({ type: "error", room: "lobby", text: "Connect first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    if (!nick.trim()) {
      pushChat({ type: "error", room: "lobby", text: "Enter nickname first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }

    const ok = safeSend({ type: "name", nick: nick.trim() });
    if (ok) {
      setNickSet(true);
      pushChat({ type: "system", room: "lobby", text: `Nickname set to "${nick.trim()}". Now join a room.`, ts: Math.floor(Date.now() / 1000) });
    }
  };

  const joinRoom = () => {
    if (!connected) {
      pushChat({ type: "error", room: "lobby", text: "Connect first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    if (!nickSet) {
      pushChat({ type: "error", room: "lobby", text: "Set nickname first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }

    const r = (room || "").trim() || "lobby";
    setRoom(r);

    const ok = safeSend({ type: "join", room: r });
    if (ok) {
      setRoomJoined(true);
      pushChat({ type: "system", room: r, text: `Joined room "${r}" ✅ (You are online here)`, ts: Math.floor(Date.now() / 1000) });
    }
  };

  const onSendPm = () => {
    if (!connected) {
      pushChat({ type: "error", room: "lobby", text: "Connect first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    if (!roomJoined) {
      pushChat({ type: "error", room: "lobby", text: "Join a room first (lobby/custom).", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    if (!pmTo.trim() || !pmMsg.trim()) {
      pushChat({ type: "error", room, text: "PM needs both 'to' and 'message'.", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    safeSend({ type: "pm", to: pmTo.trim(), text: pmMsg.trim() });
    setPmMsg("");
  };

  const onSendMessage = (e) => {
    e.preventDefault();
    if (!connected) {
      pushChat({ type: "error", room: "lobby", text: "Connect first.", ts: Math.floor(Date.now() / 1000) });
      return;
    }
    if (!roomJoined) {
      pushChat({ type: "error", room: "lobby", text: "Join a room first (lobby/custom).", ts: Math.floor(Date.now() / 1000) });
      return;
    }

    const text = msgInputRef.current?.value?.trim() || "";
    if (!text) return;

    safeSend({ type: "message", text });
    msgInputRef.current.value = "";
  };

  const onClearChat = () => setChat([]);
  const onClearTerm = () => setTerm([{ level: "warn", text: "Terminal cleared", ts: Math.floor(Date.now() / 1000) }]);

  const renderText = (m) => {
    if (!m || typeof m !== "object") return String(m);

    if (m.type === "message") return `[${m.room}] ${m.from}: ${m.text}`;
    if (m.type === "system") return `[${m.room}] ${m.text}`;
    if (m.type === "error") return `[${m.room}] ERROR: ${m.text}`;
    if (m.type === "pm") return `[${m.room}] [pm] ${m.from} -> ${m.to}: ${m.text}`;
    return JSON.stringify(m);
  };

  const kindFor = (m) => {
    if (m.type === "error") return "error";
    if (m.type === "system") return "system";
    if (m.type === "pm") return "pm";
    return "";
  };

  const identityStep = !connected ? "1) Connect" : !nickSet ? "2) Set Nickname" : !roomJoined ? "3) Join Room" : "✅ Online";

  return (
    <>
      <div className="peer-bg" aria-hidden="true">
        <span className="peer-blob pb1"></span>
        <span className="peer-blob pb2"></span>
      </div>

      <div className="peer-shell">
        <header className="peer-topbar">
          <div className="peer-brand">
            <div className="peer-mark">P</div>
            <div>
              <div className="peer-title">Peerlink</div>
              <div className="peer-subtitle">Blue borders • White UI • JSON chat • Rooms • PM • Logs</div>
            </div>
          </div>

          <div className="peer-conn">
            <div className="peer-conn-line">
              <span className={`peer-pill ${connected ? "ok" : "bad"}`}>{connected ? "CONNECTED" : "DISCONNECTED"}</span>
              <span className="peer-url">{WS_URL}</span>
            </div>

            <div className="peer-btn-row">
              <button className="peer-btn peer-primary" onClick={() => connect(false)} disabled={connected}>Connect</button>
              <button className="peer-btn peer-ghost" onClick={disconnect} disabled={!connected}>Disconnect</button>
            </div>
          </div>
        </header>

        <main className="peer-layout">
          <aside className="peer-side">
            <section className="peer-card">
              <h2 className="peer-card-title">Identity</h2>
              <p className="peer-note">
                Step: <b>{identityStep}</b><br />
                Room can be <b>lobby</b> or your custom name.
              </p>

              <div className="peer-field">
                <label className="peer-label">Nickname</label>
                <div className="peer-control">
                  <input
                    className="peer-input"
                    value={nick}
                    onChange={(e) => { setNick(e.target.value); setNickSet(false); setRoomJoined(false); }}
                    placeholder="e.g. ansh"
                    disabled={!connected}
                  />
                  <button className="peer-btn peer-outline" onClick={onSetNick} disabled={!connected}>Set</button>
                </div>
              </div>

              <div className="peer-field">
                <label className="peer-label">Room</label>
                <div className="peer-control">
                  <input
                    className="peer-input"
                    value={room}
                    onChange={(e) => { setRoom(e.target.value); setRoomJoined(false); }}
                    placeholder="lobby"
                    disabled={!connected || !nickSet}
                  />
                  <button className="peer-btn peer-outline" onClick={joinRoom} disabled={!connected || !nickSet}>Join</button>
                </div>
              </div>
            </section>

            <section className="peer-card">
              <h2 className="peer-card-title">Online Users</h2>
              <p className="peer-note">From server JSON: <code>{"{type:'users', users:[...]}"}</code></p>
              <div className="peer-chips">
                {users.length === 0 ? (
                  <span className="peer-chip">no users yet</span>
                ) : (
                  users.map((u) => (
                    <span key={u} className="peer-user">
                      <span className="peer-dot" />
                      {u}
                    </span>
                  ))
                )}
              </div>
            </section>

            <section className="peer-card">
              <h2 className="peer-card-title">Private Message</h2>
              <p className="peer-note">Enabled after you join a room.</p>

              <div className="peer-field">
                <label className="peer-label">To</label>
                <input className="peer-input" value={pmTo} onChange={(e) => setPmTo(e.target.value)} placeholder="nickname" disabled={!roomJoined} />
              </div>

              <div className="peer-field">
                <label className="peer-label">Message</label>
                <input className="peer-input" value={pmMsg} onChange={(e) => setPmMsg(e.target.value)} placeholder="type something..." disabled={!roomJoined} />
              </div>

              <button className="peer-btn peer-primary peer-wide" onClick={onSendPm} disabled={!roomJoined}>Send PM</button>

              <div className="peer-divider" />

              <div className="peer-btn-row">
                <button className="peer-btn peer-ghost peer-wide" onClick={onClearChat}>Clear Chat</button>
                <button className="peer-btn peer-ghost peer-wide" onClick={onClearTerm}>Clear Terminal</button>
              </div>

              <div className="peer-terminal" ref={scrollTermRef}>
                {term.map((l, i) => (
                  <div key={i} className={`peer-term-line ${l.level}`}>
                    [{fmtTime(l.ts)}] {l.text}
                  </div>
                ))}
              </div>
            </section>
          </aside>

          <section className="peer-chat">
            <header className="peer-chat-head">
              <div className="peer-chat-title">
                Room <span className="peer-room">{(room || "lobby").trim() || "lobby"}</span>
              </div>
              <div className="peer-note">White UI + blue borders • JSON events</div>
            </header>

            <div className="peer-messages" ref={scrollChatRef}>
              {chat.map((m, idx) => {
                const kind = kindFor(m);
                return (
                  <div key={idx} className={`peer-row ${kind}`}>
                    <div className={`peer-bubble ${kind}`}>
                      {kind === "pm" ? <span className="peer-badge">PM</span> : null}
                      {renderText(m)}
                      <span className="peer-time">{fmtTime(m.ts)}</span>
                    </div>
                  </div>
                );
              })}
            </div>

            <form className="peer-composer" onSubmit={onSendMessage}>
              <input
                ref={msgInputRef}
                className="peer-input peer-grow"
                placeholder={roomJoined ? "Write a message..." : "Join a room to enable chat..."}
                disabled={!roomJoined}
              />
              <button className="peer-btn peer-primary" type="submit" disabled={!roomJoined}>Send</button>
            </form>
          </section>
        </main>

        <footer className="peer-footer">
          <span>Peerlink • Boost.Asio + Beast + JSON</span>
          <span className="peer-sep">•</span>
          <span>Flow: Connect → Name → Join Room</span>
        </footer>
      </div>
    </>
  );
}