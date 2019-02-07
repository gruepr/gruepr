//////////////////////////////////////////////////////////////////
// gruepr
// version 7.3
// 02/07/19
//////////////////////////////////////////////////////////////////
// Copyright 2019, Joshua Hertz
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////

// Program for splitting a set of 4-200 students into optimized teams.
// Originally based on CATME's team forming routine as described in their paper:
// [ http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf ]

// All of the student data is read from a data file on the harddrive.
// The students are then split up into teams of the desired size.
// An optimized distribution of students into teams is determined by a "compatibility score."
// The compatability score can be based on:
//    1) preventing isolated women,
//    2) between 0 - 9 numerical "attribute levels", which could be skills assessments or work preferences/attitudes, and
//        can be individually chosen as to whether homogeneity or heterogeneity is desired within each team,
//    3) preventing any particular students from being on the same team,
//    4) requiring any particular students to be on the same team, and
//    5) degree of overlap in schedule freetime.

// ***REQUIRED FORMAT OF STUDENT DATAFILE***
// - header row, contains column names, including attribute questions displayed to user
// - each student on a separate row, starting at the second row immediately after the header
// - in each row, values are comma-separated, and the values cannot contain any commas (technically, the notes can contain commas...)
// - values are, in order:
//     Timestamp (ignored by the program, but must be present)
//     first name/preferred name
//     last name
//     email
//     [optional] "Woman", "Man", and any number of additional gender categories ("Prefer not to say", "Non-binary", etc.)
//     [0 to 9 values] attributes, each in own field, first numeric digit found in the value will be used, and any text before/after ignored
//     [7 values] list of timeblocks student is unavailable to work, represented as whether or not the texts given in "timeNames[]" below are included
//     [optional] section
//     [optional] additional notes for student (any and all remaining values will get swallowed into this field)

// COMPILING NOTES: Using gcc. Add C++11 compatibility and enable highest optimizations. Add linker options: "-static-libgcc  -Wl,--stack,33554432". 

// The optimization problem is very difficult (NP-hard? NP-complete?). There are, for example, almost 6E19 ways to partition 32
// students into 8 teams of 4. A genetic algorithm is used here to find a good set of teammates.
// First, each student is given an internal ID number from 0 to N-1. A large population of random teamings is then created.
// Each "genome" here represents one way to split the N students into n teams, and is represented as a permutation of the list {0, 2, 3...,N-1}.
// The population of genomes is then refined over multiple generations.
// From each generation a next generation is created.
// First, a small number of the highest scoring genomes (the "elites") are directly cloned into the next generation,
// then, the vast majority of the next generation are created by mating tournament-selected parents using order crossover (OX1),
// and, finally, a small number of new, random genomes are added to the genepool.
// Once the next generation's baseline genepool is created, mutations are allowed.
// Each genome has 1 or more potential mutations, which is a swapping of two random locations on the genome.

// Each genome is given a net score based on the compatability score of each team within the genome.
// As long as every team in the genome has a positive score, the genome's net score is the harmonic mean of the score for each team in the genome.
// The harmonic mean is used so that low scoring teams have more weight in determining the net score of the genome.
// The net score is normalized to be roughly out of 100.
// Teams with a large amount of schedule overlap can get "extra credit" resulting in scores over 100.
// Teams that do not match required criteria can get penalized resulting in scores of 0 or below.
// If any team(s) in the genome has/have a score less than or equal to zero, then the harmonic mean is mathematically problematic. Instead, what is used is the
// arithmetic mean punished by reducing it towards negative infinity by half the arithmetic mean.

// Generations continue to be created--evolution proceeds--for at least minGenerations and at most maxGenerations,
// displaying each time the generation number and the score of that generation's best genome.
// Evolution stops before maxGenerations if the best score over the last generationsOfStability generations has remained +/- 1% of the best score in the current generation.

// Once evolution has stopped, the highest scoring genome--the best set of teams in the current genepool--is shown on the screen.
// The teams are shown by listing the students' names, email addresses, indications of women students, and their attribute levels.
// Each team's score is also shown along with a table of student availability at each time slot throughout the week.
// The user can throw this away and find a new team set or, optionally, save to a file.
// If saved to a file, the screen output is saved to one file and 2 additional files are also created.
// One additional file lists the teams in the format needed to import into the TEAMMATES peer review website.
// The other additional file removes the scores and attribute data, so is useful for distributing to the students.

// Note about genetic algorithm efficiency:
// There is redundancy in the permutation-of-teammate-ID way that the teammates are encoded into the genome.
// For example, if teams are of size 4, one genome that starts [1 5 18 9 x x x x ...] and another that has
// [x x x x 9 5 1 18...] are encoding an identical team in two ways. Since every genome has teams split at the
// same locations in the array, the order crossover isn't so bad a method for creating children as long as
// genomes are split at the team boundaries. Good parents create good children by passing on what's most likely
// good about their genome--good team(s). If the crossover operation had been blind to the teammate
// boundaries, it would be less efficient, potentially even splitting up a good team if the crossover occurred in the
// middle of a preferred team. In that case, good parents would more likely lead to good children if either:
// 1) the crossover split ocurred in the middle of a bad team (helpful), 2) the crossover split ocurred at a team
// boundary (helpful, but unlikely), or 3) the crossover split a good team but other parent has exact same good team in 
// exact same location of genome (unhelpful--leads to preference for a single good genome and thus premature
// selection). Splitting always along team boundaries ensures primarily the second option happens, and thus good
// parents pass along good teams, in general, wherever they occur along the genome. However, there still are
// redundancies inherent in this encoding scheme, making it less efficient. Swapping the positions of two
// teammates within a team or of two whole teams within the list is represented by two different genomes. Also,
// teams from one parent will get split when different teams have different numbers of students.
// If writing a paper about this, compare with more severe problems suggested by Genetic Grouping Algorithm (GGA).
// WAYS THAT MIGHT IMPROVE ALGORITHM IN FUTURE:
//	-use multiple genepools and allow limited cross-breeding
//  -better memory management (arrays in heap, not stack?)
//  -use parallel processing for faster operations--especially the breeding to next generation which can be completely done in parallel
//  -use proper OOP--team is class and section is class, most functions are member functions
//	-to get around redundancy of genome issue, store each genome as an unordered_set of unordered_sets. Each team is a set of IDs; each section is a set of teams.

//////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <windows.h>

using namespace std;


////////////////////////////////////////////
// Declarations
////////////////////////////////////////////

//////////////////
// Constants
//////////////////

//max sizes of various arrays
const short maxStudents = 200;							// maximum number of students in a section (this might be changable, but program gets slower with >100 students)
const short maxAttributes = 9;							// maximum number of skills/attitudes

//console window size
const short wWidth = 120;
const short wHeight = 50;

//genetic algorithm constants:
const short populationSize = 30000;						// the number of genomes in each generation--larger size is slower, but arguably more optimized result. A size of 5000 works with the default stack size. For size of 20000, stack size was increased to 16 MB. For 30000, increased to 32 MB.
const short tournamentSize = populationSize/500;		// most of the next generation is created by mating many pairs of parent genomes, each time chosen from genomes in a randomly selected tournament in the genepool
const short topGenomeLikelihood = 33 * (RAND_MAX/100);	// first number gives probability out of 100 for selecting the best genome in the tournament as first parent to mate, then the best among the rest for second parent; if top is not selected, move to next best genome
const short numElites = 1;//populationSize/500;			// from each generation, this many highest scoring genomes are directly cloned into the next generation. Some suggest elitism helps speed genetic algorithms, but can lead to premature convergence. Having just 1 elite significantly stabilizes the high score to end optimization
const short numRandos = populationSize/500;				// this many completely random genomes are directly added into each new generation
const short minGenerations = 50;						// will keep optimizing for at least minGenerations
const short maxGenerations = 500;						// will keep optimizing for at most maxGenerations
const short generationsOfStability = 30;				// after minGenerations, if score has not improved for generationsOfStability, stop optimizing
const short mutationLikelihood = 10 * (RAND_MAX/100);	// first number gives probability out of 100 for a mutation (when mutation occurs, another chance at mutation is given with same likelihood (iteratively))

//schedule constants, *MUST* be coordinated with the Google Form question that collects this data
const short dailyTimeBlocks = 14;						// how many blocks of time are in the day?
const short numTimeBlocks = 7*dailyTimeBlocks;			// how many blocks of time are in the week?
const string dayNames[7]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const string timeNames[dailyTimeBlocks] = {"8am", "9am", "10am", "11am", "noon", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm", "7pm", "8pm", "9pm"};

//////////////////
// Structs
//////////////////

//struct defining survey data from one student, read from the input csv file
struct studentrecord
{
	string firstname;
	string lastname;
	string email;
	bool woman;
	short attribute[maxAttributes] = {0};				// rating for each attribute (each rating is numerical value from 1 -> attributeLevels[attribute])
	bool unavailable[numTimeBlocks] = {false};			// true if this is a busy block during week
	bool preventedWith[maxStudents] = {false};			// true if this student is prevented from working with the corresponding student
	bool requiredWith[maxStudents] = {false};			// true if this student is required to work with the corresponding student
	string section;										// section data stored as text
	string notes;										// any special notes for this student
};

//struct used for tournament selection of parents to mate in genetic algorithm
struct tourneyPlayer
{
	short genome[maxStudents];
	short score;
};

//handle to the output window
HANDLE window = GetStdHandle(STD_OUTPUT_HANDLE);

//////////////////
// Functions
//////////////////

//utility functions
void setWindowProps(string consoleTitle);
string openFile(ifstream& file);
string readCSVField(stringstream& row);	// move the next CSV field from row into the output string, either directly to the next comma or the comma after the entire quote-enclosed string 
void setParameters(string attribQuestionText[], bool genderQuestionIncluded, bool& isolatedWomanPrevented, short& desiredTimeBlocksOverlap, short& minTimeBlocksOverlap, short numAttributes, short numMetrics, float metricWeights[], bool desireHomogeneous[], short& meetingBlockSize);
studentrecord readOneRecordFromFile(ifstream& file, bool genderQuestionIncluded, short numAttributes, short attributeLevel[], bool sectionIncluded, bool notesIncluded);
void setTeamSizes(short numStudents, short teamSize[], short& numTeams);
void printStudentInfo(short ID, studentrecord student, short largestNameSize, short numStudents, short PrevOrReq);
void printTeams(studentrecord students[], short teammates[], short teamSize[], short numTeams, float teamScores[], string filename, short largestNameSize, short numAttributes, string sectionName);
float getTeamScores(studentrecord students[], short numStudents, short teammates[], short teamSize[], short numTeams, float teamScores[], bool isolatedWomanPrevented, short attributeLevel[], short desiredTimeBlocksOverlap, short minTimeBlocksOverlap, short numAttributes, short numMetrics, float metricWeights[], bool desireHomogeneous[], short meetingBlockSize);
void coutBold(string s);
void createWindow(string title);
void printSeparator();
void printSectionHeader(string headerText);
void startQuestion();

//genetic algorithm functions
void mate(short mom[], short dad[], short teamSize[], short numTeams, short child[], short genomeSize);	//ordered crossover production of one child from two parents, splitting allele at random team boundaries
void mutate(short genome[], short genomeSize); 			//randomly swap two elements in a genome


////////////////////////////////////////////
// Here we go.
////////////////////////////////////////////

int main()
{
	srand(time(0));

	setWindowProps(" gruepr");

	////////////////////////////////////////////
	// Get the file and read the header row information
	////////////////////////////////////////////

	// Ask for the survey data filename and check that the file is valid
	ifstream file;
	stringstream headerRow(openFile(file));			// the first line of the file (the header) is returned by openFile, so put into a stringstream for processing

    // Read past first few fields
    string field;
    field = readCSVField(headerRow);				// read past first field in header row ("Timestamp")
    field = readCSVField(headerRow);				// read past second field in header row (the question text for "First Name/Preferred Name")
    field = readCSVField(headerRow);				// read past third field in header row (the question text for "Last Name")
    field = readCSVField(headerRow);				// read past fourth field in header row (the question text for "Email")
    // Read the optional questions gender/attribute questions
    field = readCSVField(headerRow);				// read fifth field in header row
    transform(field.begin(), field.end(), field.begin(), ::tolower);	// convert to lower case
    // See if gender data is included
    bool genderQuestionIncluded=false;									// assume no gender question included and then test
    if(field.find("gender") != string::npos)
	{
		genderQuestionIncluded = true;
		field = readCSVField(headerRow);									// move on to next field
		transform(field.begin(), field.end(), field.begin(), ::tolower);	// convert to lower case
	}
    // Count the number of attributes by counting number of questions from here until one includes "Check the times". Save attribute question texts, if any, into string array.
    short numAttributes=0;													// how many skill/attitude rankings are there?
    string attribQuestionText[maxAttributes];
	while(field.find("check the times") == string::npos)
	{
		attribQuestionText[numAttributes] = field;
		numAttributes++;
		field = readCSVField(headerRow);									// move on to next field
		transform(field.begin(), field.end(), field.begin(), ::tolower);	// convert to lower case
	}
	short numMetrics = numAttributes + 1;									// total number of metrics to optimize: metrics = attributes + schedule
	// Last read should be the first time question, so read five more times to get to the last schedule question + any remaining questions
	for(short i=0; i<5; i++)
    {
    	field = readCSVField(headerRow);
    }
    // See if there is a comma in what's left, meaning there are additional field(s) after the last schedule question
    getline(headerRow, field);											    // read up to the next newline--this is the rest of the header row, including the last schedule question
    bool notesIncluded = false;
    bool sectionIncluded = false;
    if(field.find(",") != string::npos)                                  	// There is at least 1 additional field in header
    {
    	field = field.substr(field.find(","));								// read past the final schedule question
		transform(field.begin(), field.end(), field.begin(), ::tolower);	// convert to lower case
		//cout << endl << "Remaining header after final time question: " << field << endl << endl;
		if(field.find("section") != string::npos)							// next field is a section question
		{
    		//cout << "section field present." << endl << endl;
    		sectionIncluded = true;
	    	if(field.find(",", 1) != string::npos)							// if there are any more fields after section
	    	{
	    		//cout << "notes field present." << endl << endl;
	    		notesIncluded = true;
	    	}
    	}
    	else
    	{
    		//cout << "notes field present." << endl << endl;
    		notesIncluded = true;
    	}
    }


    ////////////////////////////////////////////
    // Read the student data from the file and store in array of studentrecord struct's
    ////////////////////////////////////////////

    studentrecord students[maxStudents];								// array holding each of the student's data as read from the file
    short numStudents = 0;												// counter for the number of records in the file; used to set the number of students in the section for the rest of the program
    short attributeLevel[maxAttributes]={0};							// how many levels are there in each skill/attitude ranking? Max possible is 9 due to how the values are read from the csv file (the first numeric character in field).
    getline(file, field, ',');											// first read in each row is the timestamp, so read first timestamp into dummy variable
    do
    {
        students[numStudents] = readOneRecordFromFile(file, genderQuestionIncluded, numAttributes, attributeLevel, sectionIncluded, notesIncluded);
        //cout << students[numStudents].firstname << " " << students[numStudents].lastname << endl;
        numStudents++;
        getline(file, field, ',');	// read timestamp for next row and throw away (also, in case of a newline character after last row, need this read to reach .eof())
    }
    while(!file.eof());
    file.close();
	cout << "\n        Found ";
	coutBold(to_string(numStudents));
	cout << " students";
	Beep(1568, 50); Beep(2352, 100);
	string sectionName;
	if(sectionIncluded)
	{
		// copy over all section values into sectionNames array
	    string sectionNames[numStudents];
	    short sectionSizes[numStudents];
		for(short student = 0; student < numStudents; student++)
		{
			sectionNames[student] = students[student].section;
		}
		// count and keep only unique sectionNames
		sort(sectionNames, sectionNames+numStudents);
		short numSections = 0;
		for(short student = 0; student < numStudents; student++)
		{
			//copy unique section name and increase count
			sectionNames[numSections] = sectionNames[student];
			sectionSizes[numSections] = 1;
			while((student < (numStudents-1)) && (sectionNames[student] == sectionNames[student+1]))
			{
				// move index ahead for duplicates
				student++;
				sectionSizes[numSections]++;
			}
			numSections++;
		}
		cout << " in ";
		coutBold(to_string(numSections));
		cout << " section";
		
		if(numSections > 1)
		{
			cout << "s" << endl;
			// Display section names and let user choose which section to team
			short desiredSection;
			startQuestion();
			cout << "Which section would you like to form into teams:";
			for(short section = 0; section < numSections; section++)
			{
				cout << "\n          (";
				coutBold(to_string(section+1));
				cout << ") " << sectionNames[section] << ", [";
				coutBold(to_string(sectionSizes[section]));
				cout << " students]";
			}
			cout << "\n          (";
			coutBold(to_string(numSections+1));
			cout << ") all students regardless of section, [";
			coutBold(to_string(numStudents));
			cout << " students]";
			cout << endl << endl;
			do
			{
				cout << "          (";
				for(short section = 0; section < (numSections+1); section++)
				{
					coutBold(to_string(section+1));
					if(section < numSections-1)
					{
						cout << ", ";
					}
					else if(section == numSections-1)
					{
						cout << ", or ";
					}
				}
				cout << ") ? ";
				cin >> desiredSection;
			}
			while((desiredSection < 1) || (desiredSection > (numSections+1)));

			if(desiredSection < (numSections+1))
			{
				// Move students from desired section to the front of students[] and change numStudents accordingly
				short numStudentsInSection = 0;
				for(short student = 0; student < numStudents; student++)
				{
					if(students[student].section == sectionNames[desiredSection-1])
					{
						students[numStudentsInSection] = students[student];
						numStudentsInSection++;
					}
				}
				numStudents = numStudentsInSection;
				sectionName = sectionNames[desiredSection-1];
	
				setWindowProps(" Section: [" + sectionName + "]  -  gruepr");
			}
			else
			{
				setWindowProps("All Sections  -  gruepr");
			}
		}
		else
		{
			cout << endl;
			Sleep(3000);	// just give some time for user to read info
		}
	}
	else
	{
		cout << endl;
		Sleep(3000);	// just give some time for user to read info
	}

    // Sort the students based on first name then last name
    sort(students, students+numStudents, [](studentrecord a, studentrecord b){return a.firstname < b.firstname;});
    sort(students, students+numStudents, [](studentrecord a, studentrecord b){return a.lastname < b.lastname;});
    
    // Determine the size of the largest name
    short largestNameSize = 5;											// assume largest name is (at least) 5 characters--list formatting requires at least this
    for(short student = 0; student < numStudents; student++)
    {
    	short nameSize = (students[student].firstname.size() + students[student].lastname.size() + 1); 	// adding 1 for the space between the first and last names
    	if(nameSize > largestNameSize)									//if we find a bigger name, save that value
		{
			largestNameSize = nameSize;
		}
	}
	largestNameSize = min((int)largestNameSize, 23);


    ////////////////////////////////////////////
	// Set the teaming parameters (except team size, which comes after reading how many students are in the file)
	////////////////////////////////////////////

	bool isolatedWomanPrevented;											// if true, will prevent teams with a isolated woman
	short desiredTimeBlocksOverlap;										// want at least this many time blocks per week overlapped (additional overlap is counted less schedule score)
	short minTimeBlocksOverlap;											// a team is penalized if there are fewer than this many time blocks that overlap
	float metricWeights[maxAttributes+1]={1};							// weights for each attribute plus schedule (first value defaults to 1 in case there are no attributes--need weight of 1 for schedule)
	bool desireHomogeneous[maxAttributes]; 								// if true/false, tries to make all students on a team have similar/different levels of each attribute
	short meetingBlockSize;												// count available meeting times in units of 1 hour or 2 hours long
	setParameters(attribQuestionText, genderQuestionIncluded, isolatedWomanPrevented, desiredTimeBlocksOverlap, minTimeBlocksOverlap, numAttributes, numMetrics, metricWeights, desireHomogeneous, meetingBlockSize);


	////////////////////////////////////////////
	// Determine the size, and therefore number, of teams
	////////////////////////////////////////////
	
	short numTeams;
	short teamSize[maxStudents]={0};		// max number of teams is one per student for the max number of students; initialize every team size to 0
	setTeamSizes(numStudents, teamSize, numTeams);


	////////////////////////////////////////////
	// Get any prevented and/or required teammates from user
	////////////////////////////////////////////

	// Print out formatted list of students and ask user for prevented/required student IDs
	// This runs twice - first for prevented teammates, then for required teammates
	string PrevReqBold[2] = {"PREVENTED", "REQUIRED"}, PrevReqUp[2] = {"Prevented", "Required"};
	cin.clear();
	cin.ignore(100, '\n');
	for(short PrevOrReq = 0; PrevOrReq < 2; PrevOrReq++)	// do this all twice, first to get prevented teammates and then to get required teammates
	{
		bool ask_again;				// flag for whether we will ask for another prevented/required teammate pair
		string PrevOrReqBold = PrevReqBold[PrevOrReq], PrevOrReqUp = PrevReqUp[PrevOrReq];
		short tableWidth = min((2 * largestNameSize) + 73, (int)wWidth);
		do
		{
			ask_again = false;		//assume we are done asking; will set this flag to true if user enters in a prevented/required teammate pair
			createWindow(PrevOrReqUp + " Teammates");
			// print out the name of all students in a 2-column table
			// first, print out a table header
			cout << string((wWidth - tableWidth)/2, ' ');								// spaces to center the table
			cout << left << setw(largestNameSize+9) << "  ID     Name";
			cout << right << setw(9) << PrevOrReqUp;
			cout << " Teammates        ";
			cout << (char)186;																	// column divider line
			cout << left << setw(largestNameSize+9) << "  ID     Name";
			cout << right << setw(9) << PrevOrReqUp;
			cout << " Teammates\n";
			cout << string((wWidth - tableWidth)/2, ' ') << string(largestNameSize+36, 205) << (char)206 << string(largestNameSize+36, 205) << endl;	// horizontal line
			// then, print the names in 2 columns
			for(short ID = 0; ID < ((numStudents/2) + numStudents%2); ID++)
			{
				cout << string((wWidth - tableWidth)/2, ' ');									// spaces to center the table
				printStudentInfo(ID, students[ID], largestNameSize, numStudents, PrevOrReq);	// column 1
				cout << (char)186;																// column divider line
				if((ID + (numStudents/2) + numStudents%2) < numStudents)						// column 2, unless blank last line because odd number of students
				{
					printStudentInfo(ID + (numStudents/2) + (numStudents%2), students[ID + (numStudents/2) + (numStudents%2)], largestNameSize, numStudents, PrevOrReq);
				}
				cout << endl;
			}
			cout << string((wWidth - tableWidth)/2, ' ') << string(largestNameSize+36, 205) << (char)202 << string(largestNameSize+36, 205) << endl;	// horizontal line
			startQuestion();
			cout << "Enter the ";
			coutBold("IDs");
			cout << " of a set of "<< PrevOrReqBold << " teammates, separated by spaces or commas. If none, hit [";
			coutBold("enter");
			cout << "]: ";
			//get whole line input from user and parse into separate integers
			string input;
			getline(cin, input);
			replace(input.begin(), input.end(), ',', ' ');	// replace any and all commas with spaces
			ask_again = (!input.empty());
			stringstream stream(input);
			short n[8]={0}, count=0;
			while((count < 8) && (stream >> n[count]))		// process each entered ID, sending the value from the string into the array n
			{
				if(n[count] > 0 && n[count] <= numStudents)	//if this is a valid ID, count it
				{
				  count++;
				}
			}
			if(count != 0)
			{
				for(int ID1 = 0; ID1 < count; ID1++)
				{
					for(int ID2 = ID1+1; ID2 < count; ID2++)
					{
						if(n[ID1] != n[ID2])
						{
							if(PrevOrReq == 0)
							{
								students[n[ID1]-1].preventedWith[n[ID2]-1] = true;
								students[n[ID2]-1].preventedWith[n[ID1]-1] = true;
							}
							else
							{
								students[n[ID1]-1].requiredWith[n[ID2]-1] = true;
								students[n[ID2]-1].requiredWith[n[ID1]-1] = true;
							}
						}
					}
				}
			}
		}
		while(ask_again);
	}


	////////////////////////////////////////////
	// Create and optimize teams using genetic algorithm
	////////////////////////////////////////////

	createWindow("Optimizing the Teams");
	short genePool[populationSize][numStudents], best;
	float teamScores[numTeams];
	bool dontLikeTeams;							// allow user to reject and recreate teams after viewing
	do
	{
		// Initialize an initial generation of random teammate sets, genePool[populationSize][numStudents].
		// Each genome in this generation stores (by permutation) which students are in which team.
		// Array has one entry per student and lists, in order, the "ID number" of the
		// student, referring to the order of the student in the students[] array.
		// For example, if team 1 has 4 students, and genePool[0][] = [4, 9, 12, 1, 3, 6...], then the first genome places
		// students[] entries 4, 9, 12, and 1 on to team 1 and students[] entries 3 and 6 as the first two students on team 2.

		//start with randPerm as just the sorted array {0, 1, 2, 3,..., numStudents}
		short randPerm[numStudents];
		for(short i = 0; i < numStudents; i++)
		{
			randPerm[i] = i;
		}
		//then make "populationSize" number of random permutations for initial population, store in genePool
		for(short i = 0; i < populationSize; i++)
		{
			random_shuffle(randPerm, randPerm+numStudents);
			copy(randPerm, randPerm+numStudents, &genePool[i][0]);
		}

		//now optimize
		short temp[numStudents];
		float scores[populationSize];					// total score for each genome in the gene pool
		float bestScores[generationsOfStability]={0};	// historical record of best score in the genome, going back generationsOfStability generations
		short generation = 0;
		short extraGenerations = 0;		// keeps track of "extra generations" to include in generation number displayed, used when user has chosen to continue optimizing further
		bool keepOptimizing;
		do								// allow user to choose to continue optimizing beyond maxGenerations or seemingly reaching stability
		{
			do							// keep optimizing until reach maxGenerations or stable
			{
				tourneyPlayer players[tournamentSize];
				short tourneyPicks[tournamentSize], tempGen[populationSize][numStudents];
				//calculate all of this generation's scores
				for(short i = 0; i < populationSize; i++)
				{
					scores[i] = getTeamScores(students, numStudents, &genePool[i][0], teamSize, numTeams, teamScores, isolatedWomanPrevented, attributeLevel, desiredTimeBlocksOverlap, minTimeBlocksOverlap, numAttributes, numMetrics, metricWeights, desireHomogeneous, meetingBlockSize);
				}

				//find the elites (best scores) in genePool, copy each to tempGen, then move it to the end of genePool so we don't find it again as an elite
				for(short i = 0; i < numElites; i++)
				{
					best = max_element(scores, scores+populationSize-i) - scores;	//subtract i from length because we move the best scores/genomes to the end. Thus we select the best, 2nd best, etc. by looking at one less value from end each time
					copy(&genePool[best][0], &genePool[best][0]+numStudents, &tempGen[i][0]);
					move_backward(&genePool[best][0], &genePool[best][0]+numStudents, &genePool[populationSize-i][0]);
					short swap = scores[populationSize-i-1];
					scores[populationSize-i-1] = scores[best];
					scores[best] = swap;
				}
	
				//create populationSize-numElites-numRandos children and place in tempGen
				for(short i = numElites; i < populationSize-numRandos; i++)
				{
					//get tournamentSize random values from 0 -> populationSize and copy those index-valued genePool genomes and scores into players[]
					for(short j = 0; j < tournamentSize; j++)
					{
						tourneyPicks[j] = rand()%populationSize;
						copy(&genePool[tourneyPicks[j]][0], &genePool[tourneyPicks[j]][0]+numStudents, players[j].genome);
						players[j].score = scores[tourneyPicks[j]];
					}
	
					//sort tournament genomes so top genomes in tournament are at the beginning
					sort(players, players+tournamentSize, [](tourneyPlayer i,tourneyPlayer j){return i.score>j.score;});
					
					//pick two genomes from tournament, most likely from the beginning so that best genomes are more likely have offspring
					int parent[2], choice = 0, play = 0;
					while(choice < 2)
					{
						if(rand() < topGenomeLikelihood)	//choosing 1st (i.e., best) genome with some likelihood, if not then choose 2nd, and so on; 2nd parent then chosen from remaining (lower) players in tournament
						{
							parent[choice] = play%tournamentSize;
							choice++;
						}
						play++;
					}
	
					//mate top two genomes and put child in tempGen
					mate(players[parent[0]].genome, players[parent[1]].genome, teamSize, numTeams, temp, numStudents);
					copy(temp, temp+numStudents, &tempGen[i][0]);
				}
	
				//create numRandos and put in tempGen
				for(short i = populationSize-numRandos; i < populationSize; i++)
				{
				random_shuffle(randPerm, randPerm+numStudents);
				copy(randPerm, randPerm+numStudents, &tempGen[i][0]);
				}
	
				//mutate genomes in tempGen with some probability--if a mutation occurs, mutate same genome again with same probability
				for(short i = 0; i < populationSize; i++)
				{
					while(rand() < mutationLikelihood)
					{
						mutate(&tempGen[i][0], numStudents);
					}
				}
	
				//copy all of tempGen into genePool
				copy(&tempGen[0][0], &tempGen[0][0]+populationSize*numStudents, &genePool[0][0]);
	
				//determine and display generation number and best score; save best score in historical record
				for(short i = 0; i < populationSize; i++)
				{
					scores[i] = getTeamScores(students, numStudents, &genePool[i][0], teamSize, numTeams, teamScores, isolatedWomanPrevented, attributeLevel, desiredTimeBlocksOverlap, minTimeBlocksOverlap, numAttributes, numMetrics, metricWeights, desireHomogeneous, meetingBlockSize);
				}
				best = max_element(scores, scores+populationSize) - scores;
				generation++;
				bestScores[generation%generationsOfStability] = scores[best];	//array of the best scores from the last generationsOfStability generations, wrapping around the storage location
				cout << " Generation: ";
				SetConsoleTextAttribute(window, 0x0F);	// bold
				cout  << right << setw(3) << generation+extraGenerations;
				SetConsoleTextAttribute(window, 0x07);	// unbold
				cout << ", Best score: ";
				SetConsoleTextAttribute(window, 0x0F);	// bold
				cout << fixed << setprecision(3) << scores[best];
				SetConsoleTextAttribute(window, 0x07);	// unbold
				cout << ".     \r";
			}
			while((generation < minGenerations) || ((generation < maxGenerations) && ((*max_element(bestScores,bestScores+generationsOfStability) - *min_element(bestScores,bestScores+generationsOfStability)) > (0.01*bestScores[generation%generationsOfStability]))));

			// Notify that we have completed optimization
			Beep(1568, 50); Beep(2352, 100);
			
			// Ask if user wants to keep trying to optimize
			char wantToKeepGoing;
			cout << "\n";
			startQuestion(); 
			if(generation < maxGenerations)
			{
				cout << "The score seems to be stable. Would you like to";
			}
			else
			{
				cout << "We have reached " << maxGenerations << " generations. Would you like to";
			}
			do
			{
				cout << " (";
				coutBold("c");
				cout << ")ontinue optimizing or (";
				coutBold("s");
				cout << ")how the teams? ";
				cin >> wantToKeepGoing;
			}
			while(wantToKeepGoing != 'C' && wantToKeepGoing != 'c' && wantToKeepGoing != 'S' && wantToKeepGoing != 's');
			keepOptimizing = (wantToKeepGoing == 'C' || wantToKeepGoing == 'c');
			if(keepOptimizing)
			{
				extraGenerations += generation;
				generation = 0;
				printSeparator();
				cout << "\n";
			}
		}
		while(keepOptimizing);

		// Print teams list on the screen and find out what the user wants to do
		bool redisplay;
		do
		{
			getTeamScores(students, numStudents, &genePool[best][0], teamSize, numTeams, teamScores, isolatedWomanPrevented, attributeLevel, desiredTimeBlocksOverlap, minTimeBlocksOverlap, numAttributes, numMetrics, metricWeights, desireHomogeneous, meetingBlockSize);
			printTeams(students, &genePool[best][0], teamSize, numTeams, teamScores, "", largestNameSize, numAttributes, sectionName);
	
			// Keep these teams or start over, if desired
			char wantToSave;
			printSeparator();
			cout << "\n These are the current teams.";
			startQuestion();
			cout << "Would you like to";
			do
			{
				cout << " (";
				coutBold("k");
				cout << ")eep these teams, (";
				coutBold("a");
				cout << ")djust these teams, or (";
				coutBold("s");
				cout << ")huffle and try again? ";
				cin >> wantToSave;
			}
			while(wantToSave != 'K' && wantToSave != 'k' && wantToSave != 'A' && wantToSave != 'a' && wantToSave != 'S' && wantToSave != 's');
			dontLikeTeams = (wantToSave == 'S' || wantToSave == 's');
			redisplay = (wantToSave == 'A' || wantToSave == 'a');
			if(dontLikeTeams)
			{
				cout << "\n OK, we'll try again.\n";
				startQuestion();
				cout << "Would you like to change the teaming parameters";
				char changeWeights;
				do
				{
					cout << " (";
					coutBold("y");
					cout << "/";
					coutBold("n");
					cout << ")? ";
					cin >> changeWeights;
				}
				while(changeWeights != 'Y' && changeWeights != 'y' && changeWeights != 'N' && changeWeights != 'n');
				startQuestion();
				cout << "Would you like to change the team sizes";
				char changeSizes;
				do
				{
					cout << " (";
					coutBold("y");
					cout << "/";
					coutBold("n");
					cout << ")? ";
					cin >> changeSizes;
				}
				while(changeSizes != 'Y' && changeSizes != 'y' && changeSizes != 'N' && changeSizes != 'n');
				if((changeWeights == 'Y') || (changeWeights == 'y'))
				{
					setParameters(attribQuestionText, genderQuestionIncluded, isolatedWomanPrevented, desiredTimeBlocksOverlap, minTimeBlocksOverlap, numAttributes, numMetrics, metricWeights, desireHomogeneous, meetingBlockSize);
				}
				if((changeSizes == 'Y') || (changeSizes == 'y'))
				{
					setTeamSizes(numStudents, teamSize, numTeams);
				}
	
				createWindow("Optimizing the Teams");
				cout << " The best score last time was ";
				SetConsoleTextAttribute(window, 0x0F);	// bold
				cout << fixed << setprecision(3) << bestScores[generation%generationsOfStability];
				SetConsoleTextAttribute(window, 0x07);	// unbold
				cout << ".\n";
			}
			else if(wantToSave == 'A' || wantToSave == 'a')
			{
				// Allow swapping of teammates
				cin.clear();
				cin.ignore(100, '\n');
				// Ask which two IDs to swap places
				startQuestion();
				cout << "Enter the [";
				coutBold("IDs");
				cout << "] of two students to swap places, separated by spaces or commas. If none, hit [";
				coutBold("enter");
				cout << "]: ";
				//get whole line input from user and parse into separate integers
				string input;
				getline(cin, input);
				replace(input.begin(), input.end(), ',', ' ');	// replace any and all commas with spaces
				stringstream stream(input);
				short n[2]={0}, count=0;
				while((count < 2) && (stream >> n[count]))		// process each entered ID, sending the value from the string into the array n
				{
					if(n[count] > 0 && n[count] <= numStudents)	//if this is a valid ID, count it
					{
					  count++;
					}
				}
				if(count == 2)
				{
					swap(*find(&genePool[best][0], &genePool[best][0]+numStudents, n[0]-1), *find(&genePool[best][0], &genePool[best][0]+numStudents, n[1]-1));
				}
				cin.putback('\n');			// silly, but needed for correct user input in following code because assumption is coming from a "cin >>" operation
			}
		}
		while(redisplay);
	}
	while(dontLikeTeams);


	////////////////////////////////////////////
	// Save teams list to text files, if desired, and end program
	////////////////////////////////////////////

	string filename;
	cout << "\n You can save these teams to a file, if you'd like.\n";
	startQuestion();
	cout << "To save, enter a filename. Otherwise just hit [";
	coutBold("enter");
	cout << "]: ";
	cin.clear();
	cin.ignore(100, '\n');
	getline(cin, filename);
	if(!filename.empty())
	{
		if(filename.length() <5)
		{
			filename += ".txt";
		}
		else if(filename.substr(filename.length()-4) != ".txt")
		{
			filename += ".txt";
		}
		printTeams(students, &genePool[best][0], teamSize, numTeams, teamScores, filename, largestNameSize, numAttributes, sectionName);
		cout << "        File \"" << filename << "\" saved.\n        Files for distributing to students and importing into the TEAMMATES peer review website also saved.\n";
	}

	cout << "\n\n";
	startQuestion();
	cout << "Press [";
	coutBold("enter");
	cout << "] to end the program.";
	getline(cin, filename);	//just a dummy string input

	return(0);
}


////////////////////////////////////////////
// Functions
////////////////////////////////////////////


//////////////////
// Change the window title, size, and font color
//////////////////
void setWindowProps(string consoleTitle)
{
    COORD      c = {wWidth, wHeight*20};
	SMALL_RECT r = {0, 0, wWidth-1, wHeight-1};

	SetConsoleTitle(consoleTitle.c_str());
	SetConsoleScreenBufferSize(window, c);
    SetConsoleWindowInfo(window, true, &r);
	SetConsoleTextAttribute(window, 0x07);	// "unbold" (grey) text

    return;
}


//////////////////
// Open the data file and check that it is not empty, returning the first line of data (the header row)
//////////////////
string openFile(ifstream& file)
{
	createWindow("Student Survey Data File");
	string filename, headerRow;
	bool badFile;	// if can't find file or file is empty, ask for new file
	do
	{
		badFile = false;
		startQuestion();
		cout << "What is the student survey filename (include extension if not '.csv' or '.txt')? ";
		string base_filename;
		getline(cin, base_filename);
		filename = base_filename;
		file.open(filename.c_str());
		if(file.fail())	//if a file with this exact filename cannot be found
		{
			filename = base_filename + ".csv";
			file.open(filename.c_str());	//try "base_filename".csv
			if(file.fail())					//still no?
			{
				filename = base_filename + ".txt";
				file.open(filename.c_str());	//try "base_filename".txt
				if(file.fail())
				{
					cout << "\n        ";
					SetConsoleTextAttribute(window, 0x4F);	// bold, red background
					cout << "Cannot find file!";
					SetConsoleTextAttribute(window, 0x07);	// unbold
					cout << "\n";
					Beep(440, 100);
					badFile = true;
				}
			}
		}
		if(file.is_open())
		{
			getline(file, headerRow);	// read first line (header row) and make sure file isn't empty
			if(headerRow.empty())
			{
				cout << "\n        ";
				SetConsoleTextAttribute(window, 0x4F);	// bold, red background
				cout << "File \"" << filename << "\" is empty!\n";
				SetConsoleTextAttribute(window, 0x07);	// unbold
				cout << "\n";
				Beep(440, 100);
				badFile = true;
				file.close();
			}
		}
	}
	while(badFile);

	coutBold("\n        File \"" + filename + "\" found.\n");
	setWindowProps(" File:[" + filename + "]  -  gruepr");

	return(headerRow);
}


//////////////////
// Read from the datafile one student's info
//////////////////
string readCSVField(stringstream& row)
{
	char firstchar;
	row >> firstchar;		// read the first character to see if this field is enclosed in quotation marks, indicating that commas might be found within the field

	string field;
	if(firstchar == '"')
	{
		getline(row, field, '"');	// put the text between quotation marks into field
		row >> firstchar;	// read the stream one more character, past the comma at end of this field
	}
	else if(firstchar == ',')
	{
		field = "";			//empty field
	}
	else
	{
		getline(row, field, ',');
		field = firstchar + field;
	}

	return(field);
}


//////////////////
// Read from the datafile one student's info
//////////////////
studentrecord readOneRecordFromFile(ifstream& file, bool genderQuestionIncluded, short numAttributes, short attributeLevel[], bool sectionIncluded, bool notesIncluded)
{
	studentrecord student;
	string entireRow, field;
	getline(file, entireRow);
	stringstream remainingRow(entireRow);

	// 2nd field in line; should be the first name/preferred name
	student.firstname = readCSVField(remainingRow);
	student.firstname[0] = toupper(student.firstname[0]);
	//cout << "--" << student.firstname << "  ";

	// 3rd field in line; should be the last name
	student.lastname = readCSVField(remainingRow);
	student.lastname[0] = toupper(student.lastname[0]);							// for alphabetization, make sure first character is upper-case
	//cout << "--" << student.lastname << "  ";

	// 4th field in line; should be the email
	field = readCSVField(remainingRow);
	string user = field.substr(0,field.find('@'));
	string domain = field.substr(field.find('@'));								// find the domain (all text from the @)
	transform(domain.begin(), domain.end(), domain.begin(), ::tolower);			// convert the domain to lower case
	student.email = user+domain;
	//cout << "--" << student.email << "  ";

	// optional 5th field in line; might be the gender
	if(genderQuestionIncluded)
	{
		field = readCSVField(remainingRow);
		transform(field.begin(), field.end(), field.begin(), ::tolower);		// convert to lower case
		student.woman = (field.find("woman") != string::npos);					// true if "woman" is found anywhere in text
		//cout << student.woman << "  ";
	}
	else
	{
		student.woman = false;
	}

	// optional next 9 fields in line; might be the attributes
	for(short attrib = 0; attrib < numAttributes; attrib++)
	{
		field = readCSVField(remainingRow);
		short chrctr = 0;
		while((field[chrctr] < '0' || field[chrctr] > '9') && (chrctr < field.size()))	// search through this field character by character until we find a numeric digit (or reach the end)
		{
			chrctr++;
		}
		if(field[chrctr] >= '0' && field[chrctr] <= '9')
		{
			student.attribute[attrib] = field[chrctr] - '0';							// converting number character to single digit
			//cout << "attribute " << attrib+1 << ": " << student.attribute[attrib] << "  ";
			if(student.attribute[attrib] > attributeLevel[attrib])					// attribute scores all start at 1, and this allows us to auto-calibrate the max value for each question
			{
				attributeLevel[attrib] = student.attribute[attrib];
				//cout << "Attribute " << attrib << ": now max level is " << attributeLevel[attrib] << endl;
			}
		}
	}

	// next 7 fields; should be the schedule
	for(short day = 0; day < 7; day++)
	{
		if(!(sectionIncluded || notesIncluded) && (day == 6))						// no section or notes and this is the last day, so read to end of line instead of to a comma
		{
			remainingRow >> field;
		}
		else
		{
			field = readCSVField(remainingRow);
		}
		transform(field.begin(), field.end(), field.begin(), ::tolower);				// convert to lower case (so ambivalent to AM vs am vs Am)
		for(short time = 0; time < dailyTimeBlocks; time++)
		{
			student.unavailable[(day*dailyTimeBlocks)+time] = (field.find(timeNames[time]) != string::npos);
			//cout << student.unavailable[(day*dailyTimeBlocks)+time] << "  ";
		}
	}

	// optional last fields; might be section and/or additional notes
	if(sectionIncluded)
	{
		if(notesIncluded)
		{
			// read next field for section
			field = readCSVField(remainingRow);
			// read to end of line and store as notes
			remainingRow >> student.notes;
		}
		else
		{
			// read to end of line for section
			remainingRow >> field;
		}
		student.section = field;
	}
	else if(notesIncluded)
	{
		// read to end of line for notes
		remainingRow >> student.notes;
	}
	//cout << student.section << endl;
    //cout << student.notes << endl << endl;

	return student;
}


//////////////////
// Set or change the teaming parameters
//////////////////
void setParameters(string attribQuestionText[], bool genderQuestionIncluded, bool& isolatedWomanPrevented, short& desiredTimeBlocksOverlap, short& minTimeBlocksOverlap, short numAttributes, short numMetrics, float metricWeights[], bool desireHomogeneous[], short& meetingBlockSize)
{
	createWindow("Teaming Parameters");
	if(genderQuestionIncluded)
	{
		char preventIsolatedWoman;
		printSectionHeader("Gender:");
		startQuestion();
		cout << "Should teams with one woman be";
		do
		{
			cout << " (";
			coutBold("p");
			cout << ")revented or (";
			coutBold("a");
			cout << ")llowed? ";
			cin >> preventIsolatedWoman;
		}
		while(preventIsolatedWoman != 'P' && preventIsolatedWoman != 'p' && preventIsolatedWoman != 'A' && preventIsolatedWoman != 'a');
		isolatedWomanPrevented = (preventIsolatedWoman == 'P' || preventIsolatedWoman == 'p');
		cout << endl;
		printSeparator();
	}
	bool attributesThatMatter = false;	// will only ask for the weight of schedule overlap if there is one or more attributes with a non-zero weight
	for(short attrib = 0; attrib < numAttributes; attrib++)
	{
		printSectionHeader("Skill/attitude " + to_string(attrib+1) + ":  \"" + attribQuestionText[attrib] + "\"");
		// Get weight of attribute
		startQuestion();
		cout << "What is the relative weight or importance"; 
		do
		{
			cout << " (min 0, max 100)? ";
			cin >> metricWeights[attrib];
		}
		while(metricWeights[attrib] < 0 || metricWeights[attrib] > 100);
		if(metricWeights[attrib] != 0)
		{
			// Distribution so this attribute is homogeneous or heterogeneous?
			attributesThatMatter = true;
			char diffOrSame;
			startQuestion();
			cout << "Do you prefer that all the students on a team have";
			do
			{
				cout << " (";
				coutBold("d");
				cout << ")ifferent levels or the (";
				coutBold("s");
				cout << ")ame level? ";
				cin >> diffOrSame;
			}
			while(diffOrSame != 'D' && diffOrSame != 'd' && diffOrSame != 'S' && diffOrSame != 's');
			desireHomogeneous[attrib] = (diffOrSame == 'S' || diffOrSame == 's');
		}
		cout << endl;
		printSeparator();
	}
	bool scheduleMatters = false;	// will only ask for the desired amounts schedule overlap if schedule has non-zero weight
	printSectionHeader("Schedule:");
	if(attributesThatMatter)		// if no attributes matter, then schedule is only thing that matters and we do not have to ask for its weight
	{
		startQuestion();
		cout << "What is the relative weight or importance";
		do
		{
			cout << " (min 0, max 100)? ";
			cin >> metricWeights[numMetrics-1];
		}
		while(metricWeights[numMetrics-1] < 0 || metricWeights[numMetrics-1] > 100);
		if(metricWeights[numMetrics-1] != 0)
		{
			scheduleMatters = true;
		}
		//normalization, so sum of all weights = numMetrics; note: accumulate is from <numeric> header and will sum an array
		float totalWeight = accumulate(metricWeights, metricWeights+numMetrics, 0.0)/numMetrics;
		//cout << endl << "totalWeight: " << totalWeight << endl;
		for(short metric = 0; metric < numMetrics; metric++)
		{
			metricWeights[metric] /= totalWeight;
			//cout << "Metric" << metric << ": " << metricWeights[metric] << endl;
		}
	}
	else		// all attribute weights were set to 0, so set schedule weight to full weight
	{
		metricWeights[numMetrics-1] = numMetrics;
		scheduleMatters = true;
	}
	if(scheduleMatters)
	{
//		cout << "\n We will maximize available meeting times but first:\n  -try to get all teams up to a ";
//		coutBold("desired");
//		cout << " number of meeting times and\n  -reject any teams below a ";
//		coutBold("minimum");
//		cout << " number of meeting times.\n";
		startQuestion();
		cout << "Should the length of a team meeting be at least";
		do
		{
			cout << " (";
			coutBold("1");
			cout << ") hour or (";
			coutBold("2");
			cout << ") hours? ";
			cin >> meetingBlockSize;
		}
		while(meetingBlockSize < 1 || meetingBlockSize > 2);
		startQuestion();
		cout << "What is the ";
		coutBold("desired");
		cout << " number of meeting times";
		do
		{
			cout << " (min 1, max " << numTimeBlocks << ")? ";
			cin >> desiredTimeBlocksOverlap;
		}
		while(desiredTimeBlocksOverlap < 1 || desiredTimeBlocksOverlap > numTimeBlocks);
		if(desiredTimeBlocksOverlap > 1)
		{
			startQuestion();
			cout << "What is the ";
			coutBold("minimum");
			cout << " number of meeting times";
			do
			{
				cout << " (min 0, max " << desiredTimeBlocksOverlap-1 << ")? ";
				cin >> minTimeBlocksOverlap;
			}
			while(minTimeBlocksOverlap < 0 || minTimeBlocksOverlap >= desiredTimeBlocksOverlap);
		}
		else
		{
			minTimeBlocksOverlap = 0;
		}
	}
	else
	{
		desiredTimeBlocksOverlap = numTimeBlocks;
		minTimeBlocksOverlap = 0;
	}
	
	return;
}


//////////////////
// Set or change the team sizes
//////////////////
void setTeamSizes(short numStudents, short teamSize[], short& numTeams)
{
	createWindow("Team Size");
	printSectionHeader("There are " + to_string(numStudents) + " students in this section.");

	// Ask user for the ideal team size
	short desiredTeamSize;					// how many students on the ideal team
	startQuestion();
	cout << "What is the ideal number of students on each team"; 
	do
	{
		cout << " (min 2, max " << numStudents/2 << "; enter 0 for custom team sizes)? ";
		cin >> desiredTeamSize;
	}
	while((desiredTeamSize < 2 || desiredTeamSize > numStudents/2) && (desiredTeamSize != 0));
	
	if(desiredTeamSize == 0)
	{
		// Well la-di-da, the user wants to select their own team sizes
		startQuestion();
		cout << "OK. How many students would you like on team: " << endl;
		short team = 0;
		do
		{
			do
			{
				cout << "          " << team+1 << " (students remaining = " << numStudents << "): ";
				cin >> teamSize[team];
			}
			while((teamSize[team] > numStudents) || (teamSize[team] <= 0));
			numStudents -= teamSize[team];
			team++;
		}
		while(numStudents > 0);
		numTeams = team;	
	}
	else
	{
		// Get first guess about how many teams there should be (this may be off by -1 if the number of students do not divide evenly)
		numTeams = numStudents/desiredTeamSize;
	
		// Figure out what to do if we can't evenly split the students into the desired size
		if(numStudents%desiredTeamSize != 0)
		{
			short smallerTeamSizes[maxStudents]={0}, smallerTeamsSizeA, smallerTeamsSizeB, numSmallerATeams, largerTeamSizes[maxStudents]={0}, largerTeamsSizeA, largerTeamsSizeB, numLargerATeams;
	
			// What are the team sizes when desiredTeamSize represents a maximum size?
			for(short student = 0; student < numStudents; student++)	// run through every student
			{
				(smallerTeamSizes[student%(numTeams + 1)])++;			// add one student to each team (with 1 additional team relative to before) in turn until we run out of students
				smallerTeamsSizeA = smallerTeamSizes[student%(numTeams + 1)];	// the larger of the two (uneven) team sizes
				numSmallerATeams = (student%(numTeams + 1))+1;			// the number of larger teams
			}
			smallerTeamsSizeB = smallerTeamsSizeA - 1;					// the smaller of the two (uneven) team sizes
	
			// And what are the team sizes when desiredTeamSize represents a minimum size?
			for(short student = 0; student < numStudents; student++)	// run through every student
			{
				(largerTeamSizes[student%numTeams])++;					// add one student to each team in turn until we run out of students
				largerTeamsSizeA = largerTeamSizes[student%numTeams];	// the larger of the two (uneven) team sizes
				numLargerATeams = (student%numTeams)+1;					// the number of larger teams
			}
			largerTeamsSizeB = largerTeamsSizeA - 1;					// the smaller of the two (uneven) team sizes
	
			// Ask user which team sizes to choose (or let them enter custom sizes)
			char teamSizeChoice;
			startQuestion();
			cout << "Since we can't evenly split teams of that size, should we have:";
			// display the sizes of the smaller teams 
			cout << "\n          (";
			coutBold("a");
			cout << ") ";
			if(numSmallerATeams > 0)
			{
				cout << numSmallerATeams << " team";
				if(numSmallerATeams > 1)
				{
					cout << "s";
				}
				cout << " of " << smallerTeamsSizeA << " student";
				if(smallerTeamsSizeA > 1)
				{
					cout << "s";
				}
			}
			if((numSmallerATeams > 0) && ((numTeams+1-numSmallerATeams) > 0))
			{
				cout << " + ";
			}
			if((numTeams+1-numSmallerATeams) > 0)
			{
				cout << numTeams+1-numSmallerATeams << " team";
				if((numTeams+1-numSmallerATeams) > 1)
				{
					cout << "s";
				}
				cout << " of " << smallerTeamsSizeB << " student";
				if(smallerTeamsSizeB > 1)
				{
					cout << "s";
				}
			}
			// display the sizes of the larger teams 
			cout << "\n          (";
			coutBold("b");
			cout << ") ";
			if((numTeams-numLargerATeams) > 0)
			{
				cout << numTeams-numLargerATeams << " team";
				if((numTeams-numLargerATeams) > 1)
				{
					cout << "s";
				}
				cout << " of " << largerTeamsSizeB << " student";
				if(largerTeamsSizeB > 1)
				{
					cout << "s";
				}
			}
			if(((numTeams-numLargerATeams) > 0) && (numLargerATeams > 0))
			{
				cout << " + ";
			}
			if(numLargerATeams > 0)
			{
				cout << numLargerATeams << " team";
				if(numLargerATeams > 1)
				{
					cout << "s";
				}
				cout << " of " << largerTeamsSizeA << " student";
				if(largerTeamsSizeA > 1)
				{
					cout << "s";
				}
			}
			// offer custom option
			cout << "\n          (";
			coutBold("c");
			cout << ") custom team sizes";
			cout << endl << endl;
			do
			{
			cout << "          (";
			coutBold("a");
			cout << ", ";
			coutBold("b");
			cout << ", or ";
			coutBold("c");
			cout << ") ? ";
				cin >> teamSizeChoice;
			}
			while(teamSizeChoice != 'A' && teamSizeChoice != 'a' && teamSizeChoice != 'B' && teamSizeChoice != 'b' && teamSizeChoice != 'C' && teamSizeChoice != 'c');
			if(teamSizeChoice == 'A' || teamSizeChoice == 'a')			// the user wants the smaller teams
			{
				numTeams++;										// increase the number of teams "officially"
				for(short team = 0; team < numTeams; team++)	// run through every team 
				{
					teamSize[team] = smallerTeamSizes[team];	// set the size to the smaller sizes
				}
			}
			else if(teamSizeChoice == 'B' || teamSizeChoice == 'b')		// the user wants the larger teams
			{
				for(short team = 0; team < numTeams; team++)	// run through every team
				{
					teamSize[team] = largerTeamSizes[team];		// set the size to the larger sizes
				}
			}
			else									// Well la-di-da, the user wants to select their own team sizes
			{
				startQuestion();
				cout << "OK. How many students would you like on team: " << endl;
				short team = 0;
				do
				{
					do
					{
						cout << "          " << team+1 << " (students remaining = " << numStudents << "): ";
						cin >> teamSize[team];
					}
					while((teamSize[team] > numStudents) || (teamSize[team] <= 0));
					numStudents -= teamSize[team];
					team++;
				}
				while(numStudents > 0);
				numTeams = team;	
			}
		}
		else		//students divide evenly into teams--it's the simple case
		{
			for(short team = 0; team < numTeams; team++)	// run through every team
			{
				teamSize[team] = desiredTeamSize;			// all teams are the desired size
			}
		}
	}

	return;
}


//////////////////
// Print a student's ID #, name, and any students with whom they are prevented from being/required to be teammates
//////////////////
void printStudentInfo(short ID, studentrecord student, short largestNameSize, short numStudents, short PrevOrReq)
{
	cout << " [" << setw(3) << right;
	coutBold(to_string(ID+1));
	cout << "] ";
	string name = student.firstname + " " + student.lastname;
	if((student.firstname.size()+student.lastname.size()) > 22)
	{
		name = name.substr(0,22) + (char)236;
	}

	cout << left << setfill((char)250) << setw(largestNameSize+2) << name;	// Align columns by padding with dots

	string otherIDs;
	for(short otherStudent = 0; otherStudent < numStudents; otherStudent++)
	{
		if((PrevOrReq == 0) && student.preventedWith[otherStudent])
		{
			otherIDs += (to_string(otherStudent+1) + ",");
		}
		else if((PrevOrReq == 1) && student.requiredWith[otherStudent])
		{
			otherIDs += (to_string(otherStudent+1) + ",");
		}
	}
	// if any prevented/required teammates were printed, get rid of the trailing comma
	if(!otherIDs.empty())
	{
		otherIDs.pop_back();
	}
	cout << setfill(' ') << setw(27) << otherIDs;
	return;
}


//////////////////
// Print teams (to screen or text file), including names, emails, genders, attribute scores, and notes of each student plus table of weekly availability
//////////////////
void printTeams(studentrecord students[], short teammates[], short teamSize[], short numTeams, float teamScores[], string filename, short largestNameSize, short numAttributes, string sectionName)
{
	if(filename == "")
	{
		createWindow("The Teams");
	}

	//open the output file if writing this to file
	ofstream teamFile, studentsFile, teammatesFile;
	if(filename != "")
	{
		teamFile.open(filename.c_str());
		string filename2=filename;
		filename2.insert(filename2.size()-4, "_students");
		studentsFile.open(filename2.c_str());
		filename2=filename;
		filename2.insert(filename2.size()-4, "_TEAMMATES");
		teammatesFile.open(filename2.c_str());
		teammatesFile << "Section\tTeam\tName\tEmail" << endl;
	}

	//loop through every team
	short ID = 0;
	for(short team = 0; team < numTeams; team++)
	{
		short canMeetAt[7][dailyTimeBlocks]={0};
		string output;	//text to be output to either screen or file
		output = "Team " + to_string(team+1) + "\n";
		if(filename == "")
		{
			coutBold(output);
		}
		else
		{
			teamFile << output;
			studentsFile << output;
		}
		//loop through each teammate in the team
		for(short student = 0; student < teamSize[team]; student++)
		{
			if(filename == "")
			{
				if((teammates[ID]+1) < 100)
				{
					cout << " ";
				}
				if((teammates[ID]+1) < 10)
				{
					cout << " ";
				}
				cout << " [";
				coutBold(to_string(teammates[ID]+1));
				cout <<  "]";
				output = "";
			}
			else
			{
				output = "   ";
			}
			if(students[teammates[ID]].woman)
			{
				output += "  W  ";
			}
			else
			{
				output += "     ";
			}
			for(short attrib = 0; attrib < numAttributes; attrib++)
			{
				output += to_string(students[teammates[ID]].attribute[attrib]) + "  ";
			}
			output += students[teammates[ID]].firstname;
			output += " " ;
			output += students[teammates[ID]].lastname;
			output += string(largestNameSize+2-students[teammates[ID]].firstname.size()-students[teammates[ID]].lastname.size(), ' ');		// Align columns by padding with spaces
			output += students[teammates[ID]].email;
			//output += "     ";
			//output += students[teammates[ID]].notes;
			if(filename == "")
			{
				cout << output << endl;
			}
			else
			{
				teamFile << output << endl;
				studentsFile << "   " << students[teammates[ID]].firstname << " " << students[teammates[ID]].lastname << string(largestNameSize+2-students[teammates[ID]].firstname.size()-students[teammates[ID]].lastname.size(), ' ') << students[teammates[ID]].email << endl;
				teammatesFile << sectionName << "\t" << (team+1) << "\t" << students[teammates[ID]].firstname << " " << students[teammates[ID]].lastname << "\t" << students[teammates[ID]].email << endl;
			}
			for(short day = 0; day < 7; day++)
			{
				for(short time = 0; time < dailyTimeBlocks; time++)
				{
					if(!students[teammates[ID]].unavailable[(day*dailyTimeBlocks)+time])
					{
						canMeetAt[day][time]++;
					}
				}
			}
			ID++;
		}
		if(filename == "")
		{
			cout << "Score = " << fixed << setprecision(2) << teamScores[team] << endl << "Availability:" << endl;
		}
		else
		{
			teamFile << "Score = " << fixed << setprecision(2) << teamScores[team] << endl << "Availability:" << endl;
			studentsFile << endl << "Availability:" << endl;
		}
		output = string(15, ' ');
		for(short day = 0; day < 7; day++)
		{
			output += string(2, ' ') + dayNames[day] + string(2, ' ');
		}
		if(filename == "")
		{
			cout << output << endl;
		}
		else
		{
			teamFile << output << endl;
			studentsFile << output << endl;
		}
		for(short time = 0; time < dailyTimeBlocks; time++)
		{
			output = timeNames[time] + string((14-timeNames[time].size()), ' ');
			for(short day = 0; day < 7; day++)
			{
				string percentage = to_string((100*canMeetAt[day][time])/teamSize[team]);
				output += string(5-percentage.size(), ' ') + percentage + "% ";
			}
			if(filename == "")
			{
				cout << output << endl;
			}
			else
			{
				teamFile << output << endl;
				studentsFile << output << endl;
			}
		}
		if(filename == "")
		{
			cout << endl << endl;
		}
		else
		{
			teamFile << endl <<endl;
			studentsFile << endl << endl;
		}
	}

	if(filename != "")
	{
		teamFile.close();
		studentsFile.close();
		teammatesFile.close();
	}
	return;
}


//////////////////
// Calculate team scores, returning the total score (which is, typically, the harmonic mean of all team scores)
//////////////////
float getTeamScores(studentrecord students[], short numStudents, short teammates[], short teamSize[], short numTeams, float teamScores[], bool isolatedWomanPrevented, short attributeLevel[], short desiredTimeBlocksOverlap, short minTimeBlocksOverlap, short numAttributes, short numMetrics, float metricWeights[], bool desireHomogeneous[], short meetingBlockSize)
{
	// Loop through each attribute
	float attributeScore[numAttributes][numTeams];
	short ID;
	for(short attrib = 0; attrib < numAttributes; attrib++)
	{
		ID = 0;
		for(short team = 0; team < numTeams; team++)
		{
			short maxLevel = students[teammates[ID]].attribute[attrib], minLevel = students[teammates[ID]].attribute[attrib];
			for(short student = 0; student < teamSize[team]; student++)
			{
				if(students[teammates[ID]].attribute[attrib] > maxLevel)
				{
					maxLevel = students[teammates[ID]].attribute[attrib];
				}
				if(students[teammates[ID]].attribute[attrib] < minLevel)
				{
					minLevel = students[teammates[ID]].attribute[attrib];
				}
				ID++;
			}
			attributeScore[attrib][team] = (maxLevel - minLevel) / (attributeLevel[attrib] - 1.0);	// range in team's values divided by total possible range
			if(desireHomogeneous[attrib])	//attribute scores are 0 if homogeneous and +1 if full range of values are in a team, so flip if want homogeneous
			{
				attributeScore[attrib][team] = 1 - attributeScore[attrib][team];
			}
			attributeScore[attrib][team] *= metricWeights[attrib];
			//cout << " attribute " << attrib+1 << " score = " << attributeScore[attrib][team];
		}
	}

	// Schedule scores
	float schedScore[numTeams]={0};
	ID = 0;
	for(short team = 0; team < numTeams; team++)
	{
		short firstStudentInTeam = ID;
		//cout << endl << "Team " << team+1 << ": " << endl;
		// combine each student's schedule array into a team schedule array
		bool teamAvailability[numTimeBlocks];
		for(short time = 0; time < numTimeBlocks; time++)
		{
			ID = firstStudentInTeam;
			teamAvailability[time] = true;
			for(short student = 0; student < teamSize[team]; student++)
			{
				teamAvailability[time] = teamAvailability[time] && !students[teammates[ID]].unavailable[time];	// logical "and" each student's not-unavailability
				ID++;
			}
		}
		// count how many free time blocks there are
		if(meetingBlockSize == 1)
		{
			for(short time = 0; time < numTimeBlocks; time++)
			{
				if(teamAvailability[time])
				{
					schedScore[team]++;
					//cout << "free hour: " << time;
				}
			}
		}
		else
		{
			for(short day = 0; day < 7; day++)
			{
				for(short time = 0; time < dailyTimeBlocks-1; time++)
				{
					if(teamAvailability[(day*dailyTimeBlocks)+time])
					{
						time++;
						if(teamAvailability[(day*dailyTimeBlocks)+time])
						{
							schedScore[team]++;
							//cout << "free block: " << (day*dailyTimeBlocks)+time-1];
						}
					}
				}
			}
		}
		// convert counts to a schedule score
		if(schedScore[team] > desiredTimeBlocksOverlap)			// if team has more than desiredTimeBlocksOverlap, the "extra credit" is 1/4 of the additional overlaps
		{
			schedScore[team] = 1 + ((schedScore[team] - desiredTimeBlocksOverlap) / (4*desiredTimeBlocksOverlap));
			schedScore[team] *= metricWeights[numMetrics-1];	// schedule weight is always the last entry in the weights array
		}
		else if(schedScore[team] >= minTimeBlocksOverlap)		// if team has between minimum and desired amount of schedule overlap
		{
			schedScore[team] /= desiredTimeBlocksOverlap;		// normal schedule score is number of overlaps / desired number of overlaps
			schedScore[team] *= metricWeights[numMetrics-1];	// schedule weight is always the last entry in the weights array
		}
		else													// if team has fewer than minTimeBlocksOverlap, apply penalty
		{
			schedScore[team] = -numMetrics;
		}
		//cout << schedScore[team] << endl;
	}

	// Determine adjustments for isolated woman teams
	short genderAdj[numTeams]={0};
	ID = 0;
	for(short team = 0; team < numTeams; team++)
	{
		for(short student = 0; student < teamSize[team]; student++)
		{
			if(students[teammates[ID]].woman)
			{
				genderAdj[team]++;
			}
			ID++;
		}
		if((genderAdj[team] == 1) && isolatedWomanPrevented)
		{
			genderAdj[team] = -numMetrics;
		}
		else
		{
			genderAdj[team] = 0;
		}
	}

	// Determine adjustments for prevented teammates on same team
	float prevTeammateAdj[numTeams]={0};
	short firstStudentInTeam=0;
	// Loop through each team
	for(short team = 0; team < numTeams; team++)
	{
		//loop studentA from first student in team to 2nd-to-last student in team
		for(short studentA = firstStudentInTeam; studentA < (firstStudentInTeam + (teamSize[team]-1)); studentA++)
		{
			//loop studentB from studentA+1 to last student in team
			for(short studentB = (studentA+1); studentB < (firstStudentInTeam + teamSize[team]); studentB++)
			{
				//if pairing prevented, adjustment = -numMetrics
				if(students[teammates[studentA]].preventedWith[teammates[studentB]])
				{
					prevTeammateAdj[team] = -numMetrics;
					//cout << "Team " << team << "has prevented Teammates: " << students[teammates[studentA]].firstname << " " << students[teammates[studentA]].lastname << " & " << students[teammates[studentB]].firstname << " " << students[teammates[studentB]].lastname;
				}
			}
		}
		firstStudentInTeam += teamSize[team];
	}

	// Determine adjustments for required teammates NOT on same team
	float reqTeammateAdj[numTeams]={0};
	firstStudentInTeam=0;
	// Loop through each team
	for(short team = 0; team < numTeams; team++)
	{
		//loop through all students in team
		for(short studentA = firstStudentInTeam; studentA < (firstStudentInTeam + teamSize[team]); studentA++)
		{
			//loop through ALL other students
			for(short studentB = 0; studentB < numStudents; studentB++)
			{
				//if this pairing is required
				if(students[teammates[studentA]].requiredWith[teammates[studentB]])
				{
					bool studentBOnTeam = false;
					//loop through all of studentA's current teammates
					for(short currMates = firstStudentInTeam; currMates < (firstStudentInTeam + teamSize[team]); currMates++)
					{
						//if this pairing is found, then the required teammate is on the team!
						if(teammates[currMates] == teammates[studentB])
						{
							studentBOnTeam = true;
						}
					}
					//if the pairing was not found, then adjustment = -numMetrics
					if(!studentBOnTeam)
					{
						reqTeammateAdj[team] = -numMetrics;
						//cout << students[teammates[studentA]].firstname << " " << students[teammates[studentA]].lastname << "is not paired with required teammate " << students[teammates[studentB]].firstname << " " << students[teammates[studentB]].lastname << endl;
					}	
				}                                                                                           
			}
		}
		firstStudentInTeam += teamSize[team];
	}

	//final team scores are normalized to be out of 100 (but with "extra credit" for more than desiredTimeBlocksOverlap hours w/ 100% team availability
	for(short team = 0; team < numTeams; team++)
	{
		teamScores[team] = schedScore[team] + prevTeammateAdj[team] + reqTeammateAdj[team] + genderAdj[team];
		//cout << "Team " << team+1 << ": gender = " << genderAdj[team] << ", sched = " << schedScore[team] << ", teammates = " << prevTeammateAdj[team] + reqTeammateAdj[team];
		for(short attrib = 0; attrib < numAttributes; attrib++)
		{
			teamScores[team] += attributeScore[attrib][team];
			//cout << " Attribute " << attrib+1 << " = " << attributeScore[attrib][team];
		}
		//  cout << endl;
		teamScores[team] = 100*teamScores[team] / numMetrics;
	}

	//Use the harmonic mean for the "total score"
	//This value, the inverse of the average of the inverses, is skewed towards the smaller members so that we optimize for better values of the worse teams
	float harmonicSum = 0;
	for(short team = 0; team < numTeams; team++)
	{
		//very poor teams have 0 or negative scores, and this makes the harmonic mean meaningless
		//if any teamScore is <= 0, return the arithmetic mean punished by reducing towards negative infinity by half the arithmetic mean
		if(teamScores[team] <= 0)
		{
			float mean = accumulate(teamScores, teamScores+numTeams, 0.0)/numTeams;		// accumulate() is from <numeric>, and it sums an array
			if(mean < 0)
			{
				return(mean + (mean/2));
			}
			else
			{
				return(mean - (mean/2));
			}
		}
		harmonicSum += 1/teamScores[team];
	}
	return(numTeams/harmonicSum);
}


//////////////////
// Use ordered crossover to make child from mom and dad, splitting at random team boundaries within the genome
//////////////////
void mate(short mom[], short dad[], short teamSize[], short numTeams, short child[], short genomeSize)
{
	//randomly choose two team boundaries in the genome from which to cut an allele
	short endTeam = 1+rand()%numTeams, startTeam = rand()%endTeam;	//endTeam is between 1 and number of teams, startTeam is between 0 and endTeam-1

	//Now, need to find positions in genome to start and end allele--the "breaks" before startTeam and endTeam
	short end, start, team=0, position=0;
	while(team < endTeam)
	{
		if(startTeam == team)
		{
			start = position;
		}
		//increase position by number of students in this team
		position += teamSize[team];
		end = position;
		//go to next team
		team++;
	}

	//copy all of dad into child
	copy(dad, dad + genomeSize, child);

	//remove from the child each value in mom's allele
	for(short i = 0; i < (end-start); i++)
	{
		remove(child, child + genomeSize, mom[start+i]);
	}

	//make room for mom's allele
	move_backward(child + start, child + start + genomeSize - end, child + genomeSize);

	//copy mom's allele into child
	copy(mom + start, mom + end, child + start);

	return;
}


//////////////////
// Randomly swap two sites in given genome
//////////////////
void mutate(short genome[], short genomeSize)
{
	short site1, site2;
	do
	{
		site1 = rand();
	}
	while(site1 >= genomeSize);
	do
	{
		site2 = rand();
	}
	while((site2 >= genomeSize) || (site2 == site1));
	
	swap(genome[site1], genome[site2]);
	
	return;
}


//////////////////
// Output stuff in bold
//////////////////
void coutBold(string s)
{
	SetConsoleTextAttribute(window, 0x0F);	// bold white
	cout << s;
	SetConsoleTextAttribute(window, 0x07);	// unbold
	return;
}


//////////////////
// Output the "window" title
//////////////////
void createWindow(string title)
{
	system("CLS");
	// prints the title in yellow, inside a cyan box with a cyan window-spanning horizontal below
	SetConsoleTextAttribute(window, 0x0B);	// bright cyan
	cout << "\n" << string((wWidth-4-title.size())/2, ' ') << (char)218 << string(title.size()+2, (char)196) << (char)191;	// top bar
	cout << "\n" << string((wWidth-4-title.size())/2, ' ') << (char)179;													// left pip
	SetConsoleTextAttribute(window, 0x0E);	// bright yellow
	cout << " " << title << " ";
	SetConsoleTextAttribute(window, 0x0B);	// bright cyan
	cout << (char)179;																										// right pip
	cout << "\n" << string((wWidth-4-title.size())/2, ' ') << (char)192 << string(title.size()+2, (char)196) << (char)217;	// bottom bar
	cout << "\n\n" << string(wWidth, 196);
	SetConsoleTextAttribute(window, 0x07);	// regular
	cout << "\n";

	return;
}


//////////////////
// Output a separator line
//////////////////
void printSeparator()
{
	SetConsoleTextAttribute(window, 0x0B);	// bright cyan
	cout << "\n" << string(19, 196);
	SetConsoleTextAttribute(window, 0x07);	// regular
	return;
}


//////////////////
// Output a header in bright yellow text
//////////////////
void printSectionHeader(string headerText)
{
	SetConsoleTextAttribute(window, 0x0E);	// bright yellow
	cout << "\n " << string(1,250) << headerText << "\n";
	SetConsoleTextAttribute(window, 0x07);	// regular
	return;
}


//////////////////
// Output the "start" of a question to the user
//////////////////
void startQuestion()
{
	cout << "\n     ";
	coutBold(string(1,175));
	cout << " ";
	return;
}