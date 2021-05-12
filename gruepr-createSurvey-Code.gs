// Execute this script by visiting: https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/exec
// Execute with parameters by visiting: https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/exec?title=test11&gend=true&urm=true&numattr=3&attrtext=__text__&sched=true&start=8&end=21&days=Sunday,Monday,Tuesday,Wednesday,Thursday,Friday&sect=false&sects=11,12,13,14&addl=true
// Dev execute with parameters by visiting: https://script.google.com/macros/s/AKfycbwG5i6NP_Y092fUq7bjlhwubm2MX1HgHMKw9S496VBvStewDUE/dev?title=test11&gend=true&urm=true&numattr=3&attrtext=__text__&sched=true&start=8&end=21&days=Sunday,Monday,Tuesday,Wednesday,Thursday,Friday&sect=false&sects=11,12,13,14&addl=true

function doGet(e) {
  var dataAll = e.parameter;
  
  var title = dataAll["title"];
  var includeGender = (dataAll["gend"]=="true");
  var includeURM = (dataAll["urm"]=="true");
  var numAttributes = parseInt(dataAll["numattr"],10);
  var attributeText = (dataAll["attrtext"]).split(",");
  var attributeResponses = (dataAll["attrresps"]).split(",");
  var numAttributesWOResponseText = 0;
  var includeTimezone = false;
  if('tzone' in dataAll) {
    includeTimezone = (dataAll["tzone"]=="true");
  }
  var baseTimezone = '';
  if('bzone' in dataAll) {
    baseTimezone = dataAll["bzone"];
  }
  var includeSchedule = (dataAll["sched"]=="true");
  var scheduleMeansFree = (dataAll["busy"]=="false");
  var start = parseInt(dataAll["start"],10);
  var end = parseInt(dataAll["end"],10);
  var dayNames = (dataAll["days"]).split(",");
  var includeSection = (dataAll["sect"]=="true");
  var sectionNames = (dataAll["sects"]).split(",");
  var includePrefTeammates = (dataAll["prefmate"]=="true");
  var includePrefNonTeammates = (dataAll["prefnon"]=="true");
  var numPrefMates = 1;
  if('numprefs' in dataAll) {
    numPrefMates = parseInt(dataAll["numprefs"], 10);
  }
  var includeAdditional = (dataAll["addl"]=="true");
  
  if(title=='') {
    title = 'grueprSurvey.' + Utilities.formatDate(new Date(), "GMT", "yyyy.MM.dd.HH.mm.ss");
  }
  
  var noSectionNames = sectionNames == '';
  
  var allTimezoneNames = ['1. Newfoundland', '2. Atlantic', '3. Eastern', '4. Central', '5. Mountain', '6. Pacific', '7. Alaska', '8. Hawaii–Aleutian', '9. American Samoa'];
    
  // Create a new form
  var form = FormApp.create(title);

  // Set form options
  form.setAcceptingResponses(true).setCollectEmail(false).setShuffleQuestions(false).setIsQuiz(false).setLimitOneResponsePerUser(false).setShowLinkToRespondAgain(false);
  form.setDescription('Instructions:\n\nYour response to this survey will help you be on the best possible project team.\n\nAll answers are strictly confidential, and all answers are acceptable.\n\nPlease be as honest as possible!');  
  
  // Create basic info page
  form.addPageBreakItem()
    .setTitle('First, some basic information');
  form.addTextItem()
     .setTitle('What is your first name (or the name you prefer to be called)?')
     .setRequired(true);
  form.addTextItem()
     .setTitle('What is your last name?')
     .setRequired(true);
  var textValidation = FormApp.createTextValidation()
     .setHelpText('Please provide a valid email address.')
     .requireTextIsEmail()
     .build();
  form.addTextItem()
     .setTitle('What is your email address?')
     .setValidation(textValidation)
     .setRequired(true);
  
  if(includeGender) {
    form.addListItem()
      .setTitle('With which gender do you identify?')
      .setChoiceValues(['Woman','Man','Nonbinary','Prefer Not to Answer'])
      .setRequired(true);
  }
  
  if(includeURM) {
    form.addTextItem()
      .setTitle('How do you identify your race, ethnicity, or cultural heritage?')
      .setHelpText('Example responses include "Latinx", "Multiracial", "Black", and "Pacific Islander". You may leave this question blank if you prefer not to answer.')
      .setRequired(false);
  }
  
  // Create attributes page
  if(numAttributes > 0)
  {
    form.addPageBreakItem()
      .setTitle('This set of questions is about you, your past experiences, and / or your teamwork preferences.')
      .setHelpText('All responses are acceptable.');
    var allResponseTexts=[' ', 'Yes / No', 'Yes / Maybe / No', 'Definitely / Probably / Maybe / Probably not / Definitely not', 'Strongly preferred / Preferred / Opposed / Strongly opposed', 'True / False', 'Like me / Not like me', 'Agree / Disagree', 'Strongly agree / Agree / Undecided / Disagree / Strongly disagree', '4.0 - 3.75 / 3.74 - 3.5 / 3.49 - 3.25 / 3.24 - 3.0 / 2.99 - 2.75 / 2.74 - 2.5 / 2.49 - 2.0 / Below 2.0 / Not sure, or prefer not to say', '100 - 90 / 89 - 80 / 79 - 70 / 69 - 60 / 59 - 50 / Below 50 / Not sure, or prefer not to say', 'A / B / C / D / F / Not sure, or prefer not to say', 'Very high / Above average / Average / Below average / Very low', 'Excellent / Very good / Good / Fair / Poor', 'Highly positive / Somewhat positive / Neutral / Somewhat negative / Highly negative', 'A lot of experience / Some experience / Little experience / No experience', 'Extremely / Very / Moderately / Slightly / Not at all', 'A lot / Some / Very Little / None', 'Much more / More / About the same / Less / Much less', 'Most of the time / Some of the time / Seldom / Never', 'Available / Available, but prefer not to / Not available', 'Very frequently / Frequently / Occasionally / Rarely / Never', 'Definitely will / Probably will / Probably won\'t / Definitely won\'t', 'Very important / Important / Somewhat important / Not important', 'Leader / Mix of leader and follower / Follower', 'Highly confident / Moderately confident / Somewhat confident / Not confident', ' /  /  / ', ' /  /  /  / ', ' /  /  /  /  / ', ' /  /  /  /  /  / ', ' /  /  /  /  /  /  / ', ' /  /  /  /  /  /  /  / ', ' /  /  /  /  /  /  /  /  / ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '];
    for(var attribute = 0; attribute < numAttributes; attribute++) {
      var thisResponse = attributeResponses[attribute];
      
      if(thisResponse == 0 || (thisResponse >= 33 && thisResponse != 101)) {
        numAttributesWOResponseText++;
      }
      
      if(thisResponse == 101) {
        form.addListItem()
          .setTitle('What time zone will you be based in during this class?')
          .setChoiceValues(allTimezoneNames)
          .setRequired(true);
      } else {
        var allchoices = ' ';
        if(thisResponse < allResponseTexts.length)
        {
          allchoices = (allResponseTexts[thisResponse]).split(" / ");
        }
      
        var choices=[];
        if(thisResponse > 0 && thisResponse <= 25) {
          // 'regular' responses--add a number and dot before text
          for(var i = 0; i < allchoices.length; i++) {
            choices.push(i+1 + '. ' + allchoices[i]);
          }
        } else if (thisResponse > 25 && thisResponse < 33) {
          // numerical scale responses--add just the number and a space
          for(var i = 0; i < allchoices.length; i++) {
            choices.push(i+1 + ' ');
          }
        } else {
          // unset response values--just give the user three blanks
          choices.push(' ');
          choices.push(' ');
          choices.push(' ');
        }

        form.addMultipleChoiceItem()
          .setTitle(attributeText[attribute])
          .setChoiceValues(choices)
          .showOtherOption(false)
          .setRequired(true);
      }
    }
  }
  
  // Create schedule page
  if(includeSchedule) {
    form.addPageBreakItem()
      .setTitle('Please tell us about your weekly schedule.')
      .setHelpText('Use your best guess or estimate if necessary.');
    
    if(includeTimezone) {
      form.addListItem()
        .setTitle('What time zone will you be based in during this class?')
        .setChoiceValues(allTimezoneNames)
        .setRequired(true);
    }
    
    // get the names of the times
    var allTimeNames = ['midnight', '1AM', '2AM', '3AM', '4AM', '5AM', '6AM', '7AM', '8AM', '9AM', '10AM', '11AM', 'noon', '1PM', '2PM', '3PM', '4PM', '5PM', '6PM', '7PM', '8PM', '9PM', '10PM', '11PM'];
    var startTimeName = allTimeNames[start];
    var endTimeName = allTimeNames[end];
    var timeNames = [];
    for(var time = start; time <= end; time++) {
        timeNames.push(allTimeNames[time]);
    }
    
    var schedtitle = "";
    if(scheduleMeansFree) {
      schedtitle += 'Check the times that you are FREE and will be AVAILABLE for group work.';
    } else {
      schedtitle += 'Check the times that you are BUSY and will be UNAVAILABLE for group work.';
    }
    if(includeTimezone && (baseTimezone == '')) {
      schedtitle += ' These times refer to your home timezone.';
    }
    if(baseTimezone != '') {
      schedtitle += ' These times refer to ' + baseTimezone + ' time.';
    }

    form.addCheckboxGridItem()
      .setTitle(schedtitle)
      .setHelpText('You may need to scroll to see all columns (' + startTimeName + ' to ' + endTimeName + ').')
      .setColumns(timeNames)
      .setRows(dayNames);
  }

  // Create section and other questions page
  if(includeSection || includePrefTeammates || includePrefNonTeammates || includeAdditional)
  {
    form.addPageBreakItem()
      .setTitle('Some final questions.');
    if(includeSection) {
      form.addListItem()
        .setTitle('In which section are you enrolled?')
        .setChoiceValues(sectionNames)
        .setRequired(true);
    }
    if(includePrefTeammates) {
      if(numPrefMates == 1) {
        form.addTextItem()
        .setTitle('Please write the name of someone you would like to have on your team. Write their first and last name only.')
        .setRequired(false);
      } else {
        form.addTextItem()
        .setTitle('Please list the name(s) of up to ' + numPrefMates +' people who you would like to have on your team. Write their first and last name, and put a comma between multiple names.')
        .setRequired(false);
      }
    }
    if(includePrefNonTeammates) {
      if(numPrefMates == 1) {
        form.addTextItem()
        .setTitle('Please write the name of someone you would like to NOT have on your team. Write their first and last name only.')
        .setRequired(false);
      } else {
        form.addTextItem()
        .setTitle('Please list the name(s) of up to ' + numPrefMates +' people who you would like to NOT have on your team. Write their first and last name, and put a comma between multiple names.')
        .setRequired(false);
      }
    }
    if(includeAdditional) {
      form.addTextItem()
      .setTitle('Any additional things we should know about you before we form the teams?')
      .setRequired(false);
    }
  }
  
  // Create a spreadsheet destination and get the DOCID
  var ss = SpreadsheetApp.create(title + ' (Responses)');
  form.setDestination(FormApp.DestinationType.SPREADSHEET, ss.getId());  

  var page = HtmlService.createTemplateFromFile('surveyCreated');
  page.title = title;
  page.editURL = form.getEditUrl();
  page.shortURL = UrlFetchApp.fetch('http://tinyurl.com/api-create.php?url=' + form.getPublishedUrl());
  page.ssID = form.getDestinationId();
  page.numAttributes = numAttributes;
  page.numAttributesWOResponseText = numAttributesWOResponseText;
  page.includeAdditional = includeAdditional;
  page.gender = includeGender;
  page.urm = includeURM;  
  page.sections = includeSection;
  page.noSectionNames = noSectionNames;
  return page.evaluate().setTitle('gruepr Survey Created');
}