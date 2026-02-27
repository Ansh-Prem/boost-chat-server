const WS_URL = `ws://${location.hostname}:8080`;

let ws = null;
let currentRoom = "lobby";
let currentNick = "";

const el = (id) => document.getElementById(id);

const statusPill = el("statusPill");
const btnConnect = el("btnConnect");
const btnDisconnect = el("btnDisconnect");

const nickInput = el("nickInput");
const roomInput = el("roomInput");
const roomLabel = el("roomLabel");
const wsLabel = el("wsLabel");

const btnSetNick = el("btnSetNick");
const btnJoin = el("btnJoin");
const btnPm = el("btnPm");
const btnClear = el("btnClear");

const pmToInput = el("pmToInput");
const pmMsgInput = el("pmMsgInput");

const messages = el("messages");
const sendForm = el("sendForm");
const msgInput = el("msgInput");

wsLabel.textContent = WS_URL;

function setStatus(connected) {
  if (connected) {
    statusPill.textContent = "CONNECTED";
    statusPill.style.borderColor = "rgba(39,255,154,.35)";
    statusPill.style.color = "rgba(220,255,244,.95)";
    btnConnect.disabled = true;
    btnDisconnect.disabled = false;
  } else {
    statusPill.textContent = "DISCONNECTED";
    statusPill.style.borderColor = "rgba(255,77,109,.35)";
    statusPill.style.color = "rgba(255,220,228,.95)";
    btnConnect.disabled = false;
    btnDisconnect.disabled = true;
  }
}

function addMessage(text, kind = "") {
  const row = document.createElement("div");
  row.className = "msg-row";

  const bubble = document.createElement("div");
  bubble.className = "bubble";

  if (kind === "mine") row.classList.add("mine");
  if (kind === "system") row.classList.add("system");
  if (kind === "mine") bubble.classList.add("mine");
  if (kind === "system") bubble.classList.add("system");
  if (kind === "error") bubble.classList.add("error");

  bubble.textContent = text;

  row.appendChild(bubble);
  messages.appendChild(row);
  messages.scrollTop = messages.scrollHeight;
}

function connect() {
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return;

  addMessage(`Connecting to ${WS_URL} ...`, "system");
  ws = new WebSocket(WS_URL);

  ws.onopen = () => {
    setStatus(true);
    addMessage("Connected ✅", "system");

    // join current room
    currentRoom = (roomInput.value.trim() || "lobby");
    roomLabel.textContent = currentRoom;
    ws.send(`/join ${currentRoom}`);

    // set nick if provided
    const nick = nickInput.value.trim();
    if (nick) {
      currentNick = nick;
      ws.send(`/name ${currentNick}`);
    }
  };

  ws.onmessage = (e) => {
    addMessage(e.data, "");
  };

  ws.onerror = () => {
    addMessage("WebSocket error. Is server running on port 8080?", "error");
  };

  ws.onclose = () => {
    setStatus(false);
    addMessage("Disconnected ❌", "system");
  };
}

function disconnect() {
  if (ws) ws.close();
}

function sendLine(line, showMine = false) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    addMessage("Not connected. Click Connect first.", "error");
    return;
  }
  ws.send(line);
  if (showMine) addMessage(line, "mine");
}

btnConnect.addEventListener("click", connect);
btnDisconnect.addEventListener("click", disconnect);

btnSetNick.addEventListener("click", () => {
  const nick = nickInput.value.trim();
  if (!nick) return addMessage("Enter nickname first.", "error");
  currentNick = nick;
  sendLine(`/name ${nick}`, true);
});

btnJoin.addEventListener("click", () => {
  const room = roomInput.value.trim();
  if (!room) return addMessage("Enter room name first.", "error");
  currentRoom = room;
  roomLabel.textContent = currentRoom;
  sendLine(`/join ${room}`, true);
});

btnPm.addEventListener("click", () => {
  const to = pmToInput.value.trim();
  const msg = pmMsgInput.value.trim();
  if (!to || !msg) return addMessage("PM needs both 'to' and 'message'.", "error");
  sendLine(`/pm ${to} ${msg}`, false);
  addMessage(`[pm -> ${to}] ${msg}`, "mine");
  pmMsgInput.value = "";
});

btnClear.addEventListener("click", () => {
  messages.innerHTML = "";
});

sendForm.addEventListener("submit", (e) => {
  e.preventDefault();
  const msg = msgInput.value.trim();
  if (!msg) return;
  sendLine(msg, true);
  msgInput.value = "";
});

setStatus(false);
roomLabel.textContent = roomInput.value.trim() || "lobby";
addMessage("Tip: Click Connect, set nickname, join a room, and start chatting.", "system");