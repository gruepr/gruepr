// script performs the actions not yet available directly with forms REST API: create sheet to save responses and provide URLs
// post with two parameters: id, title

function doGet(e) {
  var dataAll = e.parameter;
  
  // Retrieve form
  var form = FormApp.openById(dataAll["id"]);

  // Set form options
  form.setAcceptingResponses(true).setCollectEmail(false).setLimitOneResponsePerUser(false).setShowLinkToRespondAgain(false);
  form.setDescription('Instructions:\n\nYour response to this survey will help you be on the best possible project team.\n\nAll answers are strictly confidential, and all answers are acceptable.\n\nPlease be as honest as possible!');
  
  // Create a spreadsheet destination and get the DOCID
  var ss = SpreadsheetApp.create(dataAll["title"] + ' (Responses)');
  form.setDestination(FormApp.DestinationType.SPREADSHEET, ss.getId());  

  // Create the URLs to send back
  var editURL = form.getEditUrl();
  var responseURL = form.getPublishedUrl();
  var csvURL = "https://docs.google.com/spreadsheets/d/" + form.getDestinationId() + "/export?format=csv";

  return HtmlService.createHtmlOutput('**Success**__USERDATA__editURL='+editURL+'&csvURL='+csvURL+'&responseURL='+responseURL+'__USERDATA__');
}