/**
 * Event Counter – Google Sheets Web App
 *
 * Logs button press events from a microcontroller into a Google Sheet.
 *
 * ============================================================
 * SETUP INSTRUCTIONS
 * ============================================================
 *
 * 1. Create a new Google Spreadsheet
 *    - Add a sheet named: Event_logger
 *    - Add a sheet named: Summary
 *
 * 2. Open Extensions > Apps Script
 *    - Delete any existing code
 *    - Paste this file as code.gs
 *
 * 3. Configure the values in CONFIG below
 *    - SPREADSHEET_URL → your spreadsheet URL
 *    - TIMEZONE → your local timezone
 *
 * 4. Save the project
 *
 * 5. Click Deploy > New deployment
 *    - Select type: Web app
 *    - Execute as: Me
 *    - Who has access: Anyone
 *
 * 6. Copy the Web App URL
 *    - This URL will be used by the device
 *
 * 7. Test in a browser:
 *    ?deviceID=1&tag=test&value=1&bat=0.85
 *
 * ============================================================
 * EXPECTED QUERY PARAMETERS
 * ============================================================
 *
 * deviceID → identifier of the device
 * tag      → action label (e.g. "Drank water")
 * value    → optional numeric value
 * bat      → battery level
 *
 * ============================================================
 */

// ====== CONFIGURATION ======
const CONFIG = {
  TIMEZONE: "YOUR_TIMEZONE_HERE", // e.g. "America/Chicago"
  SPREADSHEET_URL: "YOUR_SPREADSHEET_URL_HERE",
  SHEETS: {
    DATA: "Event_logger",
    SUMMARY: "Summary"
  }
};

// ====== ENTRY POINT ======
function doGet(e) {
  try {
    const params = normalizeParams(e);
    logEvent(params);

    return textResponse(
      `Wrote:
deviceID: ${params.deviceID}
tag: ${params.tag}
value: ${params.value}
bat: ${params.bat}`
    );

  } catch (error) {
    console.error(error);
    return textResponse(`Error: ${error.message}`);
  }
}

// ====== HELPERS ======
function normalizeParams(e) {
  const defaults = {
    deviceID: "unknown",
    tag: "undefined",
    value: "",
    bat: ""
  };

  const parameters = (e && e.parameters) || {};
  return {
    deviceID: parameters.deviceID?.[0] || defaults.deviceID,
    tag: parameters.tag?.[0] || defaults.tag,
    value: parameters.value?.[0] || defaults.value,
    bat: parameters.bat?.[0] || defaults.bat
  };
}

function logEvent({ deviceID, tag, value, bat }) {
  const ss = SpreadsheetApp.openByUrl(CONFIG.SPREADSHEET_URL);
  const dataSheet = ss.getSheetByName(CONFIG.SHEETS.DATA);
  const summarySheet = ss.getSheetByName(CONFIG.SHEETS.SUMMARY);

  if (!dataSheet || !summarySheet) {
    throw new Error("Required sheets not found");
  }

  const timestamp = Utilities.formatDate(
    new Date(),
    CONFIG.TIMEZONE,
    "yyyy-MM-dd HH:mm:ss"
  );

  const nextRow = dataSheet.getLastRow() + 1;

  dataSheet.getRange(nextRow, 1, 1, 6).setValues([[
    nextRow - 1, // Event ID
    timestamp,  // Timestamp
    deviceID,   // Device ID
    bat,        // Battery level
    tag,        // Action label
    value       // Value
  ]]);

  summarySheet.getRange("B1").setValue(timestamp);
}

function textResponse(text) {
  return ContentService
    .createTextOutput(text)
    .setMimeType(ContentService.MimeType.TEXT);
}
