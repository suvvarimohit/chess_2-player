import createChessModule from "./chess_core.js";

let LOGICAL_WIDTH = 1088;
let LOGICAL_HEIGHT = 760;
let BOARD_X = 48;
let BOARD_Y = 78;
let TILE = 76;
let BOARD_SIZE = TILE * 8;
let PANEL_X = 700;
let PANEL_WIDTH = 350;
let PANEL_Y = 65;
let stackedLayout = false;
const PIECE_NAMES = ["pawn", "knight", "bishop", "rook", "queen", "king"];
const PROMOTIONS = [5, 4, 3, 2];
const PROMOTION_LABELS = ["Queen (Q)", "Rook (R)", "Bishop (B)", "Knight (N)"];

const canvas = document.querySelector("#chess-board");
const context = canvas.getContext("2d");
const loading = document.querySelector("#loading");
const images = new Map();
let module;
let game;
let state;
let selected = -1;
let flipped = false;
let pendingPromotion = null;
let scale = 1;
let offsetX = 0;
let offsetY = 0;
let dpr = 1;

function readState() {
  state = JSON.parse(module.UTF8ToString(module._chess_state_json()));
}

function imageFor(piece) {
  const side = piece > 0 ? "white" : "black";
  const name = PIECE_NAMES[Math.abs(piece) - 1];
  return images.get(`${side}_${name}`);
}

function loadImages() {
  const tasks = [];
  for (const side of ["white", "black"]) {
    for (const name of PIECE_NAMES) {
      const key = `${side}_${name}`;
      const image = new Image();
      image.src = `./assets/pieces/${key}.png`;
      images.set(key, image);
      tasks.push(
        new Promise((resolve, reject) => {
          image.onload = resolve;
          image.onerror = () => reject(new Error(`Could not load ${key}.png`));
        }),
      );
    }
  }
  return Promise.all(tasks);
}

function resizeCanvas() {
  const width = document.documentElement.clientWidth;
  const viewportHeight = window.innerHeight;
  dpr = Math.min(window.devicePixelRatio || 1, 2);
  stackedLayout = width < 760 || (width < 1000 && viewportHeight < 600);

  if (stackedLayout) {
    const sideMargin = Math.min(18, width * 0.04);
    PANEL_X = sideMargin;
    PANEL_WIDTH = width - sideMargin * 2;
    // Let the board use the phone's width. A short viewport should scroll
    // vertically instead of shrinking the squares to fit the controls too.
    TILE = Math.min(100, width / 8);
    BOARD_SIZE = TILE * 8;
    BOARD_X = (width - BOARD_SIZE) / 2;
    BOARD_Y = 48;
    PANEL_Y = BOARD_Y + BOARD_SIZE + 38;
    LOGICAL_WIDTH = width;
    LOGICAL_HEIGHT = Math.max(viewportHeight, PANEL_Y + 430);
    scale = 1;
    offsetX = 0;
    offsetY = 0;
  } else {
    LOGICAL_WIDTH = 1088;
    LOGICAL_HEIGHT = 760;
    BOARD_X = 48;
    BOARD_Y = 78;
    TILE = 76;
    BOARD_SIZE = TILE * 8;
    PANEL_X = 700;
    PANEL_WIDTH = 350;
    PANEL_Y = 65;
    scale = Math.min(width / LOGICAL_WIDTH, viewportHeight / LOGICAL_HEIGHT);
    offsetX = (width - LOGICAL_WIDTH * scale) / 2;
    offsetY = (viewportHeight - LOGICAL_HEIGHT * scale) / 2;
  }

  canvas.width = Math.max(1, Math.round(width * dpr));
  const canvasHeight = stackedLayout ? LOGICAL_HEIGHT : viewportHeight;
  canvas.height = Math.max(1, Math.round(canvasHeight * dpr));
  canvas.style.height = `${canvasHeight}px`;
}

function drawText(text, x, y, size, color, weight = 400) {
  context.fillStyle = color;
  context.font = `${weight} ${size}px "Adwaita Sans", system-ui, sans-serif`;
  context.textBaseline = "top";
  context.fillText(text, x, y);
}

function drawPiece(piece, x, y, size) {
  const image = imageFor(piece);
  if (image?.complete && image.naturalWidth) {
    context.drawImage(image, x, y, size, size);
  }
}

function drawButton(x, y, width, height, label, active = false) {
  context.fillStyle = active ? "#4b6754" : "#2f3740";
  context.fillRect(x, y, width, height);
  context.strokeStyle = "#5c6870";
  context.lineWidth = 1;
  context.strokeRect(x + 0.5, y + 0.5, width - 1, height - 1);
  drawText(label, x + 12, y + 9, 16, "#efede4");
}

function drawBoard() {
  for (let displayRank = 0; displayRank < 8; displayRank += 1) {
    for (let displayFile = 0; displayFile < 8; displayFile += 1) {
      const file = flipped ? 7 - displayFile : displayFile;
      const rank = flipped ? displayRank : 7 - displayRank;
      const square = rank * 8 + file;
      let color = (file + rank) & 1 ? "#688977" : "#e8e0ca";
      if (state.last[0] === square || state.last[1] === square) color = "#bfae67";
      if (selected === square) color = "#cdbe66";
      if (state.check && Math.abs(state.board[square]) === 6 &&
          (state.board[square] > 0 ? 0 : 1) === state.turn) {
        color = "#be5b52";
      }
      context.fillStyle = color;
      context.fillRect(BOARD_X + displayFile * TILE, BOARD_Y + displayRank * TILE, TILE, TILE);
    }
  }

  if (selected >= 0) {
    const targets = new Set();
    for (const move of state.legal) {
      if (move[0] === selected) targets.add(move[1]);
    }
    for (const to of targets) {
      const displayFile = flipped ? 7 - (to & 7) : to & 7;
      const rank = to >> 3;
      const displayRank = flipped ? rank : 7 - rank;
      const x = BOARD_X + displayFile * TILE;
      const y = BOARD_Y + displayRank * TILE;
      const isCapture = state.board[to] !== 0 ||
        (Math.abs(state.board[selected]) === 1 && (selected & 7) !== (to & 7));
      if (isCapture) {
        context.beginPath();
        context.arc(x + TILE / 2, y + TILE / 2, TILE * 0.43, 0, Math.PI * 2);
        context.strokeStyle = "rgba(41, 54, 45, 0.72)";
        context.lineWidth = 5;
        context.stroke();
      } else {
        context.beginPath();
        context.arc(x + TILE / 2, y + TILE / 2, TILE * 0.12, 0, Math.PI * 2);
        context.fillStyle = "rgba(41, 54, 45, 0.57)";
        context.fill();
      }
    }
  }

  for (let square = 0; square < 64; square += 1) {
    const piece = state.board[square];
    if (!piece) continue;
    const file = square & 7;
    const rank = square >> 3;
    const displayFile = flipped ? 7 - file : file;
    const displayRank = flipped ? rank : 7 - rank;
    drawPiece(piece, BOARD_X + displayFile * TILE, BOARD_Y + displayRank * TILE, TILE);
  }

  for (let i = 0; i < 8; i += 1) {
    if (stackedLayout) {
      drawText(String.fromCharCode(97 + (flipped ? 7 - i : i)),
        BOARD_X + i * TILE + 4, BOARD_Y + BOARD_SIZE - 15, 11, "#344239", 600);
      drawText(String(1 + (flipped ? i : 7 - i)), BOARD_X + 4,
        BOARD_Y + i * TILE + 3, 11, "#344239", 600);
    } else {
      drawText(String.fromCharCode(97 + (flipped ? 7 - i : i)),
        BOARD_X + i * TILE + TILE - 15, BOARD_Y + BOARD_SIZE + 2, 12, "#414d44");
      drawText(String(1 + (flipped ? i : 7 - i)), BOARD_X - 19,
        BOARD_Y + i * TILE + TILE - 18, 12, "#414d44");
    }
  }

  context.strokeStyle = "#12161a";
  context.lineWidth = 2;
  context.strokeRect(BOARD_X + 1, BOARD_Y + 1, BOARD_SIZE - 2, BOARD_SIZE - 2);
}

function drawPanel() {
  if (stackedLayout) {
    const x = PANEL_X;
    const width = PANEL_WIDTH;
    const y = PANEL_Y;
    drawText("C H E S S", x, y, 21, "#ebdbb8", 600);
    if (width >= 360)
      drawText("LOCAL TWO-PLAYER", x + width - 150, y + 5, 13, "#a7b3b8");

    context.fillStyle = "#242b32";
    context.fillRect(x, y + 32, width, 74);
    if (state.status) {
      drawText(state.status, x + 12, y + 43, 17, "#efcc81", 600);
      drawText("Press F2 for a new game", x + 12, y + 76, 14, "#bec6c4");
    } else {
      drawText(state.turn === 0 ? "WHITE TO MOVE" : "BLACK TO MOVE",
        x + 12, y + 43, 19, "#f1ede0", 600);
      drawText(state.check ? "Check!" : "Select a piece, then a destination",
        x + 12, y + 76, 13, state.check ? "#f58b7c" : "#b4bebc");
    }

    drawText("CAPTURED", x + 4, y + 116, 12, "#a5b2b1", 600);
    for (let side = 0; side < 2; side += 1) {
      for (let i = 0; i < state.captured[side].length; i += 1) {
        const piece = (side === 0 ? 1 : -1) * state.captured[side][i];
        drawPiece(piece, x + side * Math.floor(width / 2) + 4 + (i % 8) * 20,
          y + 134 + Math.floor(i / 8) * 20, 19);
      }
    }

    const historyY = y + 182;
    context.fillStyle = "#20262c";
    context.fillRect(x, historyY, width, 146);
    drawText("MOVE HISTORY", x + 12, historyY + 9, 13, "#a5b2b1", 600);
    const recent = state.history.slice(-8);
    for (let i = 0; i < recent.length; i += 1) {
      const half = i % 2;
      const row = Math.floor(i / 2);
      if (!half) drawText(`${Math.floor((state.history.length - recent.length + i) / 2) + 1}.`,
        x + 12, historyY + 34 + row * 22, 13, "#819193");
      drawText(recent[i], x + (half ? width * 0.57 : 48), historyY + 34 + row * 22,
        14, "#e8e7db");
    }

    const buttonsY = historyY + 158;
    const gap = 8;
    const buttonWidth = (width - gap) / 2;
    drawButton(x, buttonsY, buttonWidth, 42, "New game  F2");
    drawButton(x + buttonWidth + gap, buttonsY, buttonWidth, 42, "Undo move  U");
    drawButton(x, buttonsY + 50, width, 40, "Flip board  F", flipped);
    return;
  }

  drawText("C H E S S", BOARD_X, 24, 22, "#ebdbb8", 600);
  drawText("LOCAL TWO-PLAYER", PANEL_X, 29, 14, "#a7b3b8");

  context.fillStyle = "#242b32";
  context.fillRect(PANEL_X, 65, PANEL_WIDTH, 102);
  if (state.status) {
    drawText(state.status, PANEL_X + 18, 82, 19, "#efcc81", 600);
    drawText("Press F2 to start another game", PANEL_X + 18, 118, 15, "#bec6c4");
  } else {
    drawText(state.turn === 0 ? "WHITE TO MOVE" : "BLACK TO MOVE",
      PANEL_X + 18, 82, 21, "#f1ede0", 600);
    drawText(state.check ? "Check!" : "Select a piece, then a destination",
      PANEL_X + 18, 119, 15, state.check ? "#f58b7c" : "#b4bebc");
  }

  drawText("CAPTURED", PANEL_X + 4, 190, 13, "#a5b2b1", 600);
  for (let side = 0; side < 2; side += 1) {
    for (let i = 0; i < state.captured[side].length; i += 1) {
      const piece = (side === 0 ? 1 : -1) * state.captured[side][i];
      drawPiece(piece, PANEL_X + side * 170 + 4 + (i % 8) * 20,
        210 + Math.floor(i / 8) * 22, 20);
    }
  }

  context.fillStyle = "#20262c";
  context.fillRect(PANEL_X, 270, PANEL_WIDTH, 338);
  drawText("MOVE HISTORY", PANEL_X + 14, 280, 14, "#a5b2b1", 600);
  const firstPair = state.history.length > 24 ? Math.ceil((state.history.length - 24) / 2) : 0;
  const pairCount = Math.ceil(state.history.length / 2);
  for (let pair = firstPair; pair < pairCount; pair += 1) {
    const rowY = 310 + (pair - firstPair) * 23;
    drawText(`${pair + 1}.`, PANEL_X + 14, rowY, 14, "#819193");
    drawText(state.history[pair * 2], PANEL_X + 53, rowY, 15, "#e8e7db");
    if (pair * 2 + 1 < state.history.length) {
      drawText(state.history[pair * 2 + 1], PANEL_X + 153, rowY, 15, "#e8e7db");
    }
  }

  drawButton(PANEL_X, 650, 160, 48, "New game  F2");
  drawButton(PANEL_X + 178, 650, 172, 48, "Undo move  U");
  drawButton(PANEL_X, 708, 350, 44, "Flip board  F", flipped);
}

function drawPromotion() {
  if (!pendingPromotion) return;
  context.fillStyle = "rgba(10, 13, 17, 0.69)";
  context.fillRect(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT);
  if (stackedLayout) {
    const popupWidth = Math.min(330, LOGICAL_WIDTH - 24);
    const popupX = (LOGICAL_WIDTH - popupWidth) / 2;
    const popupY = BOARD_Y + BOARD_SIZE / 2 - 56;
    const cellWidth = popupWidth / 4;
    context.fillStyle = "#2b3239";
    context.fillRect(popupX, popupY, popupWidth, 112);
    context.strokeStyle = "#d8c28f";
    context.lineWidth = 2;
    context.strokeRect(popupX + 1, popupY + 1, popupWidth - 2, 110);
    drawText("Choose promotion", popupX + 12, popupY + 10, 16, "#f5efdc", 600);
    for (let i = 0; i < PROMOTIONS.length; i += 1) {
      const piece = PROMOTIONS[i] * (state.turn === 0 ? 1 : -1);
      drawPiece(piece, popupX + i * cellWidth + (cellWidth - 36) / 2,
        popupY + 34, 36);
      drawText(["Q", "R", "B", "N"][i], popupX + i * cellWidth + cellWidth / 2 - 4,
        popupY + 76, 12, "#e8dfc8", 600);
    }
    return;
  }
  const x = BOARD_X + BOARD_SIZE / 2 - 165;
  const y = BOARD_Y + BOARD_SIZE / 2 - 56;
  context.fillStyle = "#2b3239";
  context.fillRect(x, y, 330, 112);
  context.strokeStyle = "#d8c28f";
  context.lineWidth = 2;
  context.strokeRect(x + 1, y + 1, 328, 110);
  drawText("Choose promotion", BOARD_X + BOARD_SIZE / 2 - 84,
    BOARD_Y + BOARD_SIZE / 2 - 47, 16, "#f5efdc", 600);
  for (let i = 0; i < PROMOTION_LABELS.length; i += 1) {
    drawText(PROMOTION_LABELS[i], BOARD_X + BOARD_SIZE / 2 - 148 + i * 74,
      BOARD_Y + BOARD_SIZE / 2 + 2, 12, "#e8dfc8");
  }
}

function draw() {
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.clearRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = "#191e24";
  context.fillRect(0, 0, canvas.width, canvas.height);
  context.setTransform(dpr * scale, 0, 0, dpr * scale, dpr * offsetX, dpr * offsetY);
  drawBoard();
  drawPanel();
  drawPromotion();
}

function update() {
  readState();
  draw();
}

function playSelectedPromotion(piece) {
  if (!pendingPromotion) return;
  module._chess_play(pendingPromotion.from, pendingPromotion.to, piece);
  pendingPromotion = null;
  selected = -1;
  update();
}

function handleBoardClick(x, y) {
  if (state.status || x < BOARD_X || x >= BOARD_X + BOARD_SIZE ||
      y < BOARD_Y || y >= BOARD_Y + BOARD_SIZE) return;
  const displayFile = Math.floor((x - BOARD_X) / TILE);
  const displayRank = Math.floor((y - BOARD_Y) / TILE);
  const file = flipped ? 7 - displayFile : displayFile;
  const rank = flipped ? displayRank : 7 - displayRank;
  const target = rank * 8 + file;

  const choices = state.legal.filter((move) => move[0] === selected && move[1] === target);
  if (choices.length) {
    if (choices.some((move) => move[2] !== 0)) {
      pendingPromotion = { from: selected, to: target };
      draw();
    } else {
      module._chess_play(selected, target, 0);
      selected = -1;
      update();
    }
    return;
  }

  if (state.board[target] !== 0 &&
      (state.board[target] > 0 ? 0 : 1) === state.turn) {
    selected = target;
  } else {
    selected = -1;
  }
  draw();
}

function handleClick(event) {
  const x = (event.clientX - offsetX) / scale;
  const y = (event.clientY + window.scrollY - offsetY) / scale;

  if (pendingPromotion) {
    if (stackedLayout) {
      const popupWidth = Math.min(330, LOGICAL_WIDTH - 24);
      const popupX = (LOGICAL_WIDTH - popupWidth) / 2;
      const popupY = BOARD_Y + BOARD_SIZE / 2 - 56;
      const cellWidth = popupWidth / 4;
      if (y >= popupY + 30 && y < popupY + 112 &&
          x >= popupX && x < popupX + popupWidth) {
        const index = Math.floor((x - popupX) / cellWidth);
        playSelectedPromotion(PROMOTIONS[index]);
      }
      return;
    }
    for (let i = 0; i < 4; i += 1) {
      const left = BOARD_X + BOARD_SIZE / 2 - 132 + i * 66;
      if (x >= left && x < left + 66 &&
          y >= BOARD_Y + BOARD_SIZE / 2 - 36 && y < BOARD_Y + BOARD_SIZE / 2 + 36) {
        playSelectedPromotion(PROMOTIONS[i]);
        return;
      }
    }
    return;
  }

  const newGameRect = stackedLayout
    ? { x: PANEL_X, y: PANEL_Y + 340, w: (PANEL_WIDTH - 8) / 2, h: 42 }
    : { x: PANEL_X, y: 650, w: 160, h: 48 };
  const undoRect = stackedLayout
    ? { x: PANEL_X + (PANEL_WIDTH + 8) / 2, y: PANEL_Y + 340, w: (PANEL_WIDTH - 8) / 2, h: 42 }
    : { x: PANEL_X + 178, y: 650, w: PANEL_WIDTH - 178, h: 48 };
  const flipRect = stackedLayout
    ? { x: PANEL_X, y: PANEL_Y + 390, w: PANEL_WIDTH, h: 40 }
    : { x: PANEL_X, y: 708, w: PANEL_WIDTH, h: 44 };
  if (x >= newGameRect.x && x < newGameRect.x + newGameRect.w &&
      y >= newGameRect.y && y < newGameRect.y + newGameRect.h) {
    module._chess_reset();
    selected = -1;
    pendingPromotion = null;
    flipped = false;
    update();
    return;
  }
  if (x >= undoRect.x && x < undoRect.x + undoRect.w &&
      y >= undoRect.y && y < undoRect.y + undoRect.h) {
    module._chess_undo();
    selected = -1;
    update();
    return;
  }
  if (x >= flipRect.x && x < flipRect.x + flipRect.w &&
      y >= flipRect.y && y < flipRect.y + flipRect.h) {
    flipped = !flipped;
    draw();
    return;
  }
  handleBoardClick(x, y);
}

function handleKey(event) {
  const key = event.key.toLowerCase();
  if (event.key === "F2") {
    event.preventDefault();
    module._chess_reset();
    selected = -1;
    pendingPromotion = null;
    flipped = false;
    update();
  } else if (key === "u") {
    module._chess_undo();
    selected = -1;
    pendingPromotion = null;
    update();
  } else if (key === "f") {
    flipped = !flipped;
    draw();
  } else if (event.key === "Escape") {
    selected = -1;
    pendingPromotion = null;
    draw();
  } else if (pendingPromotion) {
    const promotion = { q: 5, r: 4, b: 3, n: 2 }[key];
    if (promotion) playSelectedPromotion(promotion);
  }
}

async function start() {
  try {
    module = await createChessModule();
    game = module;
    await Promise.all([loadImages(), document.fonts.ready]);
    readState();
    resizeCanvas();
    loading.hidden = true;
    draw();
    window.addEventListener("resize", () => {
      resizeCanvas();
      draw();
    });
    canvas.addEventListener("pointerdown", handleClick);
    window.addEventListener("keydown", handleKey);
  } catch (error) {
    loading.textContent = `Could not start chess: ${error.message}`;
    console.error(error);
  }
}

start();
