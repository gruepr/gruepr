gruepr

Copyright (C) 2019-2025, Joshua Hertz < info@gruepr.com >, Giovanni Assad, Nikhen Nyo

---------------
Description of gruepr:

     Gruepr is a program for splitting a section of 4-1000 students into optimized teams.
     It was inspired by CATME's team forming routine as described in their paper
     [ http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf ].

     Data about the students are collected and the students are split into teams of any desired size(s). A
     good distribution of students into teams is determined by a numerical score. The score can be based on:
        1) preventing isolated women, isolated men, isolated non-binary persons, and/or single-gender teams;
        2) preventing isolated URM students;
        3) achieving within each team either homogeneity or heterogeneity of up to 15 "attributes", which 
           could be skills assessments, work preferences, attitudes, major, topic preference(s), or any 
           other question that is answered by selecting one or more values from a limited set of 
           possibilities;
        4) requiring each team to have at least one student with a particular attribute;
        5) preventing students with incompatible attributes from being on the same team;
        6) achieving a high degree of overlap in schedule freetime (with timezone awareness);
        7) preventing any particular students from being on the same team;
        8) requiring any particular students to be on the same team; and/or
        9) requiring at least a certain number of students from a particular list to be on the same team.

     After the optimization process runs for some time, the best set of teams found is shown on the screen.
     The teams are displayed showing the students names, emails, gender, URM status, and attribute values.
     Each team's score is also shown along with a table of student availability at each time slot throughout
     the week. You can choose whether to save this teamset, adjust this teamset by rearranging teams or
     students, or to change the teaming options and try again. If you save the teamset, three files can be
     saved: 1) an instructor's file containing all the team and student information; 2) a student's file
     showing the same but without listing team scores or student demographics/attributes; and 3) the
     section, team, and student names in a spreadsheet format.

     The student data are typically collected using a survey that the students fill out. Gruepr will help
     the instructor create this survey, outputting the survey as either text files, a Canvas quiz, or, more
     commonly, a Google Form on the instructor's Google Drive. After collecting the students' survey
     responses, the results are loaded into gruepr. If using the Google Form, the results can be directly
     imported. If an alternate surveying instrument is used, the results must be as a comma-separated-values
     (.csv) file with each question as a separate column and each student as a separate row.

     Integration with various learning management systems is currently in development. Canvas integrations
     currently use a user-generated API token. Gruepr can create the survey as an ungraded quiz in the
     Canvas course, can directly import the survey results, and can upload the created teams as groups in
     the Canvas course.

     COMPILING NOTES: Need C++17 and OpenMP on all systems. Needs OpenSSL on Windows. Enabling the speed
     optimization switch -O2 seems to offer significant speed boost; -O3 does not seem to offer any
     improvement.

---------------
Details on how the teams are optimized:

     To optimize the teams, a genetic algorithm is used. First, a large population of 30,000 random teamings
     (each is a "genome") is created and then refined over multiple generations. In each generation, a small
     number of the highest scoring "elite" genomes are directly copied (cloned) into the next generation,
     and the rest are created by mating tournament-selected parents using ordered crossover. Once the next
     generation's genepool is created, each genome has 1 or more potential mutations, which is a swapping of
     two random locations on the genome.

     A genome's net score is the harmonic mean of the score for each team. Harmonic mean is used so that low
     scoring teams have more weight. Evolution proceeds for at least minGenerations and at most
     maxGenerations, displaying generation number and the score of that generation's best genome. Evolution
     stops (user can choose to keep it going) when the best score has remained +/- 1% for
     generationsOfStability generations or when maxGenerations is reached.


---------------
A Note about genetic algorithm efficiency:

     Unfortunately, there is redundancy in the array-based permutation-of-teammate-ID way that the teammates
     are encoded into the genome. For example, if teams are of size 4, one genome that starts
     [1 5 18 9 x x x x ...] and another that has [x x x x 9 5 1 18...] are encoding an identical team in two
     ways. Since every genome has teams split at the same locations in the array, the ordered crossover
     isn't so bad a method for creating children since genomes are split at the team boundaries. Good
     parents create good children by passing on what's most likely good about their genome--good team(s). If
     the crossover were blind to the teammate boundaries, it would be less efficient, potentially even
     splitting up a good team if the crossover occurred in the middle of a preferred team. Good parents
     would more likely lead to good children if either: 1) the crossover split ocurred in the middle of a
     bad team (helpful), 2) the crossover split ocurred at a team boundary (helpful, but unlikely), or
     3) the crossover split a good team but other parent has exact same good team in exact same location of
     genome (unhelpful--leads to preference for a single good genome and thus premature selection).
     Splitting always along team boundaries ensures primarily the second option happens, and thus good
     parents pass along good teams, in general, wherever they occur along the genome. However, there still
     are redundancies inherent in this encoding scheme, making it less efficient. Swapping the positions of
     two teammates within a team or of two whole teams within the list is represented by two different
     genomes. Additional inefficiencies are suggested by the Genetic Grouping Algorithm (GGA).


---------------
DISCLAIMER:

     This program is free software: you can redistribute it and/or modify it under the terms of the GNU
     General Public License as published by the Free Software Foundation, either version 3 of the License,
     or (at your option) any later version.

     This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
     Public License for more details.

     You should have received a copy of the GNU General Public License along with this program.  If not, see
     < https://www.gnu.org/licenses/ >.

     This software incorporates code from the open source Qt libraries, using version 6.7. These can be
     freely downloaded from < http://qt.io/download >.

     Some icons were originally created by Icons8 < https://icons8.com >. These icons have been made
     available under the creative commons license: Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0). Other
     icons and graphics are original creations for the gruepr project by
     Scout < https://scout.camd.northeastern.edu/ >.

     Several embedded fonts are used:
     Oxygen Mono, (C) 2012 Vernon Adams (vern@newtypography.co.uk);
     DM Sans (C) 2014-2017 Indian Type Foundry (info@indiantypefoundry.com); and
     Paytone One (C) 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont).
     All fonts are licensed according to the SIL OPEN FONT LICENSE Version 1.1; 
