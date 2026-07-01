## v13.0.2 — 2026-07-01

- bugfix: crash when directly loading Canvas survey results with 1+ students w/o a submission and survey has 1+ attribute question
- bugfix: crash when the teamsize means only one team will be in the teamset and optimization begins
- several other pointer or array de-referencing bugs caught

## v13.0.0 — 2026-04-17 — Major Update
- Pulling in incredible dissertation work by Nikhen Nyo to improve the user interface:
  - added visualization to show how well each team meets each team formation goal
  - turned teaming criteria into individual 'cards' that are added and rearranged to set priority order
  - UI modernizations and simplifications in start dialog and data loading process
  - diversity criteria for multiple choice questions now optimizes for most number of values (in addition to widest range of values for ordered questions)
- added several types of questions: numerical response, ranked options (such as selecting your preference(s) for a project topic), and free text response
- significantly expanded the ability to make teaming rules based on gender or racial/ethnic identity
- unified "required" and "requested" teammates
- changed handling of gender, allowing it to be multi-valued
- fixed errors with Google connection and authorization related to IPv6 networking and PKCE authorization
- improved optimization routine
- much faster load times for windows and dialogs
- updated Qt to v6.9.1, c++ to c++20 using more RAII architecture
- large number of additional bugfixes and improvements
- added experimental linux build

## v12.7.0 — 2025-12-22
- significant speedup in optimization algorithm (now supports sections of up to 1000 students!)
- significant speedup in loading data
- UI improvements:
  - the order of the teams now persists when closing/reopening the app
  - export (save/print) format more flexible
  - better display of teams when teaming all sections separately
- Canvas integration improvements:
  - auto-downloads section data with the survey
  - simplified login window
  - courses sorted by creation date, rather than alphabetically
- Bug fixes:
  - fixed crash when roster file has invalid entries
  - fixed crash when question text has a quotation mark in it
- many behind-the-scenes code updates

## v12.6.0 — 2025-09-10
- updated Qt libraries to 6.7.0
- improved software registration process
- various small bugfixes and stability improvements

## v12.5.0 — 2025-02-05
- improved handling of data that has only a single section included
- improved handling of students with a blank section (incl. students on roster without survey)
- removed unnecessary string conversions that corrupted accented and other non-ASCII characters
- more robust connection to Google, with auto-retry of missed download attempts
- no longer saves tokens and usernames in plaintext

## v12.4.0 — 2025-01-26
- refined the Google authorization and connection internals, leading to better performance

## v12.3.0 — 2025-01-19
- fixed crash when trying to form teams after loading a file without ever selecting a section
- fixed some (but not all) the Google authorization and connection issues; updated connection UI
- now maintains visibility / palette even with macOS in dark mode
- now allows selection of which google drive account to use, new forms will associate with the account used
- uses more secure PKCE setting when connecting to Google API
- much more nicely handles error upon loading survey files with < 4 responses
- (Internal) createed an LMS class with derived classes for Canvas & Google (& future LMSes); reorganized files into an LMS subfolder

## v12.2.0 — 2024-12-29
- many small improvements and bugfixes in the user interface
- fixed several bugs when comparing a roster to the student list
- now automatically retrieves names and email addresses when downloading survey submissions from Canvas
- many internal code unifications and simplifications

## v12.1.0 — 2024-10-22
- gruepr now auto-saves all of your work when creating teams; when forming teams, offers to re-load previously saved work
- upon opening a survey that has preferred teammates and / or non-teammates, gruepr now asks whether to auto-load these preferences as required and / or prevented
- more granular scheduling options, down to the 15 minute level
- better UI on when close button is hit in surveymaker, confirming close if survey has not yet been exported
- a number of smaller bugfixes and improvements

## v12.0.0 — 2024-09-21 — Major Update
- completely redone user interface, designed in partnership with a team from Scout [ https://scout.camd.northeastern.edu/ ]
- moved codebase to Qt version 6.5
- moved Windows compiler to msvc
- improved and increased integrations with Canvas
- a very large number of bugfixes and internal improvements

## v11.1.0 — 2023-11-18
- bugfix: corrected regex recognition of the word "and" in the preferred-teammates and preferred-non-teammates fields of a student record by requiring a space on either side of the word (so doesn't match, e.g., "Sanders")
- bugfix: corrected the shifting of schedules when students are submitting a schedule based on their own home timezone, including when there are 7 days in the schedule so need to wrap around Saturday <--> Sunday
- updated UI in the required attributes and incompatible attributes dialogs by making text and selection areas scrollable (needed when # of values is large, and thus window is too tall for screen)
- updated UI for the add student and edit student dialogs by making scrollable (needed most when a select-multiple-options attribute has a lot of options, and thus window is too tall for screen)
- added option to edit schedule in the add student and edit student dialogs (including flipping an existing schedule)
- added option to form teams from all students in all sections, but separated by section
- added timeout to canvas and google connections
- added Canvas quiz upload to surveymaker's create survey UI buttons

## v11.0.0 — 2023-08-10 — Major Update
- fully integrated SurveyMaker and gruepr with Canvas and Google (NOTE: Canvas in beta--for now uses a user-generated token instead of OAuth token)
- moved all https calls to be direct from program instead of thru browser, removing all use of browser except OAuth login as needed
- create Canvas survey (quiz) and Google Form directly from surveymaker
- download results of Canvas survey and Google Form directly from gruepr
- upload created teams to Canvas
- saves and uses refresh tokens to reduce need for re-authorization every time
- moved Windows binary to non-static build with OpenSSL
- implemented an installation program for Windows now that non-static build
- enable custom attribute response options from within SurveyMaker
- when adjusting schedules to class timezone, now uses the same style of time name (e.g., "1:00pm" vs "13:00") from the survey within the UI
- improved/corrected display timezone in tooltips and table displays, especially when the value is unknown
- now correctly handles attributes that are both ordered and multi-valued ("multiordered")

## v10.9.0 — 2023-06-08
- gender categories can now be based on biological sex (e.g., female), adult gender (e.g., woman), child gender (e.g., girl), or pronouns (e.g., she-her)

## v10.8.0 — 2023-05-25
- windows and macOS binaries are now code-signed, which increases security and should reduce warnings on installation
- now can undo/redo when manually adjusting the teams using drag-and-drop
- behind-the-scenes code reorganization for future integration with Canvas and more direct Google integration

## v10.7.1
- increased max number of students to 500; works fast enough with 500 students, though it does get pretty slow when class size is very large and Required Teammates or Non-Teammates are used
- adjusted scoring when team has more-than-desired schedule overlap to prevent runaway scores with high overlap
- when a student has misspelled another student's name that they've given in their survey response as a preferred teammate or non-teammate, the select-matching-name-dialogue now includes the requesting student's name
- improved the graph of optimization progress--now colors plot blue/pink when there is/is not at least one genome that meets all the rules
- improved UI in start window, with message confirming user has the latest version and is registered

## v10.7.0
- now only asks for the class's base timezone if the students were asked for their home timezone AND for their schedule
- significantly expanded the algorithm that recognizes timezone data in the survey
- Bugfix - now correctly displays at full width the UI elements in the teamset tabs
- updated C++ code standard to C++17, removing several deprecated QT elements in the process

## v10.6.1
- Bugfix - fixed crash on mac when changing the matched name in the select-matching-name dialog
- Bugfix - now correctly handles sub-version numbers when checking for new versions on startup

## v10.6.0
- upon opening gruepr, an online check of the latest downloadable version is made, offering to load gruepr homepage if a newer version is available
- the count of how many students gave each response now updates in the attributeTabs when changing the section being placed on teams
- an indicator of the maximum value of schedule and attribute weights has been added
- Bugfix - attribute tabs in gruepr now show at proper width

## v10.5.0
- much better UI with the attribute tabs in both gruepr and surveymaker
- in gruepr, the attribute tabs now show a count of how many students gave each of the responses
- in gruepr, when the attribute is home timezone, the offset from GMT is shown instead of a letter as the response value
- in surveymaker, tabs can now be moved or closed in order to quickly rearrange or remove attribute questions in the survey
- Bugfix - adding a new student now correctly sets their schedule as ambiguous
- Bugfix - action menu in required / prevented / requested teammates dialog correctly begins the correct action
- Bugfix - saving and loading a survey in surveymaker now remembers attributes that allow multiple response options

## v10.4.0
- Major feature - now each time you create teams, a brand new Teams Tab opens to show the team set created
- multiple team sets can be viewed simultaneously on different tabs. This can be used to compare easily the teams you get using different teaming options, or perhaps you'd like to create the teams for a few different sections and keep them all open before saving / printing them.
- now can load Prevented Teammates directly from an open Teams Tab. This can be used so that multiple team sets can be created with the same students and same teaming options, but where everyone gets all new teammates in each team set.
- Bugfix - team scores were miscalculated in certain rare cases
- the "actions" in the required / prevented / requested teammates dialog now only get applied to the students listed in the table (i.e., the students in the section being considered) as suggested by the wording
- as always, internal code cleanup and modernization--optimization might be slightly faster

## v10.3.0
- Bugfix - prevent crash when loading a survey without timestamp, first name, last name, and/or email address
- Bugfix - prevent crash when using the edit/add student dialog with certain attribute data
- Bugfix - improved handling of accented characters in student names and other survey data
- added new select-multiple-options type of attribute:
  - added as option in SurveyMaker, adjusting the output accordingly in the Google Form or textfile survey
  - added display in gruepr when loading survey data with an attribute question that allowed multiple responses
  - updated handling of attribute data throughout gruepr to accommodate multiple response values
  - much better UI in attribute questions within the edit/add student dialog
- improved handling of the minimum and desired numbers of meeting times:
  - values can now be equal
  - meeting block size of 3 hours now possible
  - the maximum values now adjust for the number of possible meeting times, given the survey data and meeting block size

## v10.2.0
- Bugfix - editing a student's record no longer erases the notes field
- Bugfix - removing a student record no longer messes up all of the previously entered required / prevented / requested teammates
- Bugfix - now correctly implements required / incompatible attributes with value of "unknown / not set"
- when comparing student list to roster file, now uses both name and email address to find a match; eliminated the choice of whether to use roster or survey email address when they match
- csv files can now be header-row-free
- teaming options files now include the required attribute responses
- added randomize team names function
- better UI in required / incompatible attributes dialog
- removed "OK" button from small "setting up printer/pdfwriter" dialog
- sets/resets background color of teams in table after swapping or moving students
- organized code into dialogs and widgets subfolders
- changed the internal ID assigned to each studentRecord to be a permanent value set according to a dataOption counter that increments each time a student is added

## v10.1.0
- Added a "Required Attribute" feature, which ensures each team has at least one student with a particular attribute value
- Teams with less than a 0 compatibility score now indicated with highlight color and Tooltip
- Continuing behind-the-scenes code reorganization

## v10.0.6
- Improved functionality and much better UI in the Compare To Roster feature
- Continuing behind-the-scenes code reorganization

## v10.0.5
- Bugfix - students with ambiguous schedules were not being correctly accounted in the schedule score

## v10.0.4
- Bugfix - student schedules were shifting even when a time zone was specified for the schedule
- better display of time zones in student & team info tables
- minor code reorganization

## v10.0.3
- Bugfix - prevent crash when loading a survey file that doesn't have at least one schedule or attribute question

## v10.0.2
- Bugfix - crash on loading saved teaming options

## v10.0.1
- Added to the help menu an option to submit a bug report (using BitBucket Issues tracker)

## v10.0.0

- New Features:
  - can now ask students in the survey for names of preferred teammates and / or non-teammates, and can import those in gruepr as required/requested/prevented
  - new "load roster" option to compare against survey data, updating email addresses and offering to add missing students and remove extra students
  - added timezone functionality to survey: this includes allowing students to fill out survey schedule in their own timezone and then correct/shift into the "class timezone" each after collecting the results
  - can now now prevent isolating nonbinary students, with distinction between students who are nonbinary vs. those whose gender is not known
- Better Data Handling:
  - expansion of what gets recognized for gender response: woman OR female -> woman; man OR male -> man; anything that includes "trans", "queer", or any combination of "non" and "binary" -> nonbinary
  - doesn't mark students as duplicate just because they both have blank email adresses
  - use a survey import dialog box so the user can select the meaning of each question
  - allow for section question later in the "additional questions"
  - change incompatible response dialog / behavior to add incompatible same responses (e.g., 1 with 1)
  - add a penalty point for each incompatible attribute pair found on a team
  - improved scoring of teams with one student
- Bug Fixes:
  - newlines within a csv field are now handled properly
  - re-saving a survey file after adding or editing a student mishandled the attribute values
  - the dialog box to choose which race/ethnicity to consider as URM crashed in certain cases
  - starting the optimization crashed upon first updating the progress values whenever optimization was too quick
- Better User Interface:
  - add/edit student info dialog much improved; attribute response texts now always show in attribute value selection box
  - changed display of team info to be always on, but bold and with colored background
  - put attribute questions in a tab-style display for both surveymaker and gruepr
- Behind the Scenes Improvements:
  - made interface work much better with dark mode / dark color schemes
  - LOTS of code modernization and organization
  - moved code to Qt v5.15 standards

## v9.13.1
- Reverted genetic algorithm mutation behavior, as it works much better for large genomes
- Code cleanup
- Moved macOS binaries to use Qt version 5.15; now compatible with macOS 11 (Big Sur)

## v9.13.0
- Made progress dialog modal
- Adjusted genetic algorithm such that check for mutation to occur happens with low probability per gene site, instead of high probability per genome with iteration

## v9.12.1
- Lots of code modernization and some bugfixes

## v9.12.0
- Bugfix and improvements in handling when students have ambiguous schedules (all times checked or all unchecked)

## v9.11.0
- Vastly more quick and efficient handling of data in the team info display
- Other UI improvements throughout

## v9.10.0
- Internal code modernization and optimization

## v9.9.0
- Much improved drag-and-drop functionality to modify teams after creation
- Updated UI on custom teamsizes, custom teamnames, and URMselection dialog boxes

## v9.8.3
- Fixed Print Teams dialog crash
- Still more code modernization and optimization

## v9.8.2
- Fixed crash when all students on a team have a value of "unknown" for an attribute
- Fixed crash when changing the desired team size after forming teams and then re-displaying the team info
- Further code modernization and optimization

## v9.8.1
- Fixed a few small bugs related to attribute values
- Allowed that the current teaming options are saved as new default values every time the program exit, without the popup window

## v9.8.0
- Significantly improved speed of optimization through better coding :)
- Made more flexible the recognition of schedule questions in the survey header
- Fixed bug in recognizing attribute response values that are categorical yet begin with a decimal value (e.g., GPA)
- Further modernizing of code

## v9.7.1
- Further cleaned up UI
- Fixed bug that prevented macOS version from running in certain cases

## v9.7.0
- The schedule question can now be phrased so that it asks students when they are free OR when they are busy (SurveyMaker can create the question either way and the data is correctly interpreted in gruepr).
- Cleaned up UI somewhat, and menu bar added.
- Cleaned up and simplified the optimization and genetic algorithm code

## v9.6.1
- Incompatible Attributes are now active even if that attribute has a weight of 0.
- Minor UI improvements to the Incompatible Attributes dialog.

## v9.6.0
- added external window display of optimization progress and cleaned up UI
- made the section question of the survey to be more specific
- increased the max # of attribute questions to 15
- initial order of students on each team is now alphabetical by name, and initial order of teams is by name of first student in team; click on name column gets back to this order
- add option to save surveymaker output to textfile/csv file instead of Google Form
- moved the "clear all teammates" command in the prevented/required/requested teammates window to less error-prone location
- added command to save/load CSV of prevented/required/requested teammates, using levenshtein::distance to match names
- fixed bug with multiple notes columns in survey data
- now saves/loads teaming options into a file instead of registry
- now auto-stops optimization on timer after outcome is stable
- modernize rand() and other code clean up / modernization

## v9.5.0
- Incompatible Attributes are now active even if that attribute has a weight of 0.
- Minor UI improvements to the Incompatible Attributes dialog.

## v9.4.0
- Treatment of race/ethnicity is now more inclusive. The survey question (if included) now asks students to self-identify their racial/ethnic/cultural heritage, and then allows the instructor to select later which of the responses should be considered an underrepresented minority group.

## v9.3.2
- Bugfix: crash when optimizing a dataset with all survey responses giving the same value for one or more attributes (e.g., all surveys answer "2" for the first attribute question)
- Generalized the Incompatible Responses feature so that each attribute has a list of incompatible pairs
- Continuing code modernization/cleanup

## v9.3.1
- Minor bugfixes: gruepr failing when reading in a blank value in the csv datafile, surveymaker loading incorrect attribute response options from a saved survey file

## v9.3.0
- Added option for Incompatible Responses to each attribute

## v9.2.0
- When adding Prevented Teammates, can now load a spreadsheet file from previous teaming in order to give all students new teammates
- Can now save (or overwrite) a survey file from within gruepr after adding, editing, or removing a student from the dataset
- Fixed bug in removing a single required/prevented/requested teammate
- New splashscreen during app startup

## v9.1.0
- Reorganized the "save/print teams" dialog box
- Added teaming info to the instructor's file

## v9.0.0 — Major Update
- Merged gruepr and SurveyMaker into a single app. On program startup, a window asks which to run.

## v8.17.0
- Much better handling of purely categorical (i.e., non-numerical or ordered) attributes, such as academic major.
- SurveyMaker now uses local day names, instead of defaulting to English names

## v8.16.1
- Critical bugfixes and improvements to the required/prevented/requested teammates feature.
- Improved flexibility to alternative date and time formats and character sets.
- Parallel processing during optimization brought to macOS; optimization now much faster on multi-core CPUs.

## v8.16.0
- Tables displaying student and team info are smarter in how they display information--columns can be sorted by clicking on the top of the column.
- A number of internal changes to the code to improve efficiency.

## v8.15.0
- SurveyMaker can now save surveys to the harddrive for later editing or reuse.
- Attribute questions can now have >9 response options, and the options do not need to be numbered.
- Optimization is now significantly faster on a Windows machine with a modern (multi-core) CPU through use of parallel processing; maximum number of students has been accordingly increased to 300.

## v8.14.0
- There's now a simpler way in gruepr to add a student, plus a way to edit an existing student's information.
