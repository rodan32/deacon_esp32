function doGet(e) {
  var sheetId = 'YOUR_SPREADSHEET_ID_HERE'; // The ID from your Google Sheet URL
  var ss = SpreadsheetApp.openById(sheetId);
  var today = new Date();
  today.setHours(0, 0, 0, 0); // Normalize to midnight

  var result = {
    "cfm": getComeFollowMeData(ss, today),
    "activities": getActivitiesData(ss, today),
    "assignments": getAssignmentsData(ss, today)
  };

  return ContentService.createTextOutput(JSON.stringify(result)).setMimeType(ContentService.MimeType.JSON);
}

function getComeFollowMeData(ss, today) {
  var sheet = ss.getSheetByName("ComeFollowMe");
  if (!sheet) return null;

  var data = sheet.getDataRange().getValues();
  var res = { "current": null, "next": null };

  for (var i = 1; i < data.length; i++) {
    var row = data[i];
    if (!(row[0] instanceof Date) || !(row[1] instanceof Date)) continue;

    var weekStart = new Date(row[0]);
    var weekEnd = new Date(row[1]);
    weekStart.setHours(0, 0, 0, 0);
    weekEnd.setHours(23, 59, 59, 999);

    if (today >= weekStart && today <= weekEnd) {
      res.current = {
        "start": Utilities.formatDate(weekStart, Session.getScriptTimeZone(), "MMM d"),
        "end": Utilities.formatDate(weekEnd, Session.getScriptTimeZone(), "MMM d"),
        "topic": row[2] ? row[2].toString() : "",
        "scripture": row[3] ? row[3].toString() : ""
      };

      if (i + 1 < data.length) {
        var nextRow = data[i + 1];
        if (nextRow[0] instanceof Date && nextRow[1] instanceof Date) {
          res.next = {
            "start": Utilities.formatDate(new Date(nextRow[0]), Session.getScriptTimeZone(), "MMM d"),
            "end": Utilities.formatDate(new Date(nextRow[1]), Session.getScriptTimeZone(), "MMM d"),
            "topic": nextRow[2] ? nextRow[2].toString() : "",
            "scripture": nextRow[3] ? nextRow[3].toString() : ""
          };
        }
      }
      break;
    }
  }
  return res;
}

function getActivitiesData(ss, today) {
  var sheet = ss.getSheetByName("Activities");
  if (!sheet) return null;

  var data = sheet.getDataRange().getValues();
  var res = { "current": null, "next": null };

  for (var i = 1; i < data.length; i++) {
    var row = data[i];
    if (!(row[0] instanceof Date)) continue;

    var actDate = new Date(row[0]);
    actDate.setHours(23, 59, 59, 999); // Activity happens on this date

    // We look for the first activity that hasn't happened yet (i.e. today or in the future)
    if (actDate >= today) {
      res.current = {
        "date": Utilities.formatDate(actDate, Session.getScriptTimeZone(), "MMM d"),
        "title": row[1] ? row[1].toString() : "",
        "time": row[2] ? (row[2] instanceof Date ? Utilities.formatDate(row[2], Session.getScriptTimeZone(), "h:mm a") : row[2].toString()) : "",
        "location": row[3] ? row[3].toString() : ""
      };

      if (i + 1 < data.length) {
        var nextRow = data[i + 1];
        if (nextRow[0] instanceof Date) {
          res.next = {
            "date": Utilities.formatDate(new Date(nextRow[0]), Session.getScriptTimeZone(), "MMM d"),
            "title": nextRow[1] ? nextRow[1].toString() : "",
            "time": nextRow[2] ? (nextRow[2] instanceof Date ? Utilities.formatDate(nextRow[2], Session.getScriptTimeZone(), "h:mm a") : nextRow[2].toString()) : "",
            "location": nextRow[3] ? nextRow[3].toString() : ""
          };
        }
      }
      break;
    }
  }
  return res;
}

function getAssignmentsData(ss, today) {
  var sheet = ss.getSheetByName("Assignments");
  if (!sheet) return null;

  var data = sheet.getDataRange().getValues();
  var res = { "current": null, "next": null };

  for (var i = 1; i < data.length; i++) {
    var row = data[i];
    if (!(row[0] instanceof Date)) continue;

    var asgDate = new Date(row[0]);
    asgDate.setHours(23, 59, 59, 999);

    if (asgDate >= today) {
      res.current = {
        "date": Utilities.formatDate(asgDate, Session.getScriptTimeZone(), "MMM d"),
        "lesson": row[1] ? row[1].toString() : "TBD",
        "messenger": row[2] ? row[2].toString() : "TBD"
      };

      if (i + 1 < data.length) {
        var nextRow = data[i + 1];
        if (nextRow[0] instanceof Date) {
          res.next = {
            "date": Utilities.formatDate(new Date(nextRow[0]), Session.getScriptTimeZone(), "MMM d"),
            "lesson": nextRow[1] ? nextRow[1].toString() : "TBD",
            "messenger": nextRow[2] ? nextRow[2].toString() : "TBD"
          };
        }
      }
      break;
    }
  }
  return res;
}
