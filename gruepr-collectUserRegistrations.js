// Link to execute this script: https://script.google.com/macros/s/AKfycbwqGejEAumqgwpxDdXrV5CJS54gm_0N_du7BweU3wHG-XORT8g/exec


function doGet(e) {
  // LEGACY CODE, USED BY GRUEPR v10.x AND EARLIER
  // Execute this function, with http GET, passing parameters: https://script.google.com/macros/s/AKf...T8g/exec?name=__NAME__&institution=__INSTITUTION&email=__EMAIL__

  var date = new Date();
  var dataAll = e.parameter;
  var name = dataAll["name"];
  var institution = dataAll["institution"];
  var email = dataAll["email"];

  if(name != undefined)
  {
    var FileIterator = DriveApp.getFilesByName('gruepr Users');
    while (FileIterator.hasNext())
    {
      var file = FileIterator.next();
      if (file.getName() == 'gruepr Users')
      {
        var sheet = SpreadsheetApp.open(file).getSheets()[0];
        sheet.appendRow([date, name, institution, email]);
      }
    }

    return HtmlService.createHtmlOutput('<body style="background-image: url(https://drive.google.com/uc?export=download&id=1HDa1TQToA1fqWD2y0CC6xMBHsdvnUWWz); background-repeat: no-repeat; background-size: auto; background-position: top center"><p style="visibility: hidden;">**Registration successful**__USERDATA__'+'name='+name+';institution='+institution+';email='+email+'__USERDATA__</p><h2 style="text-align: center; color: white; text-shadow: 1px 1px 0 #000, -1px 1px 0 #000, 1px -1px 0 #000, -1px -1px 0 #000, 0px 1px 0 #000, 0px -1px 0 #000, -1px 0px 0 #000, 1px 0px 0 #000, 2px 2px 0 #000, -2px 2px 0 #000, 2px -2px 0 #000, -2px -2px 0 #000, 0px 2px 0 #000, 0px -2px 0 #000, -2px 0px 0 #000, 2px 0px 0 #000, 1px 2px 0 #000, -1px 2px 0 #000, 1px -2px 0 #000, -1px -2px 0 #000, 2px 1px 0 #000, -2px 1px 0 #000, 2px -1px 0 #000, -2px -1px 0 #000;">Thank you for registering your copy of gruepr!</h2><p style="text-align: center; color: white; text-shadow: 1px 1px 0 #000, -1px 1px 0 #000, 1px -1px 0 #000, -1px -1px 0 #000, 0px 1px 0 #000, 0px -1px 0 #000, -1px 0px 0 #000, 1px 0px 0 #000, 2px 2px 0 #000, -2px 2px 0 #000, 2px -2px 0 #000, -2px -2px 0 #000, 0px 2px 0 #000, 0px -2px 0 #000, -2px 0px 0 #000, 2px 0px 0 #000, 1px 2px 0 #000, -1px 2px 0 #000, 1px -2px 0 #000, -1px -2px 0 #000, 2px 1px 0 #000, -2px 1px 0 #000, 2px -1px 0 #000, -2px -1px 0 #000;">You can close this window.</p>');
  }
  else
  {
    return HtmlService.createHtmlOutput('Undefined registration data. Please try to register again. :(');
  }
}



function doPost(e) {

  // Execute this function, with http POST, passing parameters as JSON

  var date = new Date();
  var dataAll = JSON.parse(e.postData.contents);
  var name = dataAll.name;
  var institution = dataAll.institution;
  var email = dataAll.email;
  var os = dataAll.os;


  if(name != undefined)
  {
    var FileIterator = DriveApp.getFilesByName('gruepr Users');
    while (FileIterator.hasNext())
    {
      var file = FileIterator.next();
      if (file.getName() == 'gruepr Users')
      {
        var sheet = SpreadsheetApp.open(file).getSheets()[0];
        sheet.appendRow([date, name, institution, email, os]);
      }
    }

    return HtmlService.createHtmlOutput('**Registration successful**__USERDATA__'+'name='+name+';institution='+institution+';email='+email+'__USERDATA__');
  }
  else
  {
    return HtmlService.createHtmlOutput('**Error--please try again at a later time**');
  }
}
