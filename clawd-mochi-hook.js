#!/usr/bin/env node
/**
 * Clawd Mochi — Claude Code Hook Script (USB Serial)
 *
 * Sends Claude Code state to ESP32 Clawd Mochi via USB serial (COM10).
 * No WiFi needed — direct USB connection.
 *
 * Usage: node clawd-mochi-hook.js <event_name>
 * Reads stdin JSON from Claude Code.
 */

const { execSync } = require("child_process");

const COM_PORT = "COM10";
const BAUD = 115200;

// Serial commands:
// w = normal eyes (idle)
// s = squish eyes (working)
// d = code view / terminal
// a = logo reveal
// q = quit terminal

function sendSerial(cmd) {
  try {
    // Use PowerShell to write to COM port on Windows
    const ps = [
      `$port = New-Object System.IO.Ports.SerialPort '${COM_PORT}',${BAUD},None,8,One`,
      `$port.Open()`,
      `Start-Sleep -Milliseconds 100`,
      `$port.Write('${cmd}')`,
      `Start-Sleep -Milliseconds 100`,
      `$port.Close()`,
    ].join("; ");
    execSync(`powershell.exe -ExecutionPolicy Bypass -Command "${ps}"`, {
      timeout: 5000,
      stdio: "pipe",
    });
  } catch (e) {
    // Silently fail — ESP32 might not be connected
  }
}

async function handleEvent(eventName) {
  let input = "";
  for await (const chunk of process.stdin) {
    input += chunk;
  }

  switch (eventName) {
    case "PreToolUse":
    case "PostToolUse":
    case "SubagentStart":
    case "UserPromptSubmit":
      sendSerial("s");  // squish eyes = working
      break;

    case "PostToolUseFailure":
    case "StopFailure":
      sendSerial("d");  // code view = error
      break;

    case "Stop":
    case "SubagentStop":
    case "Elicitation":
    case "Notification":
      sendSerial("w");  // normal eyes = idle
      break;

    case "SessionStart":
      sendSerial("a");  // logo reveal
      break;

    case "SessionEnd":
      sendSerial("w");  // back to idle
      break;

    case "PreCompact":
    case "PostCompact":
      break;  // no-op
  }
}

const eventName = process.argv[2];
if (!eventName) {
  process.exit(0);
}

handleEvent(eventName).then(() => process.exit(0));
