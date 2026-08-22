gruepr

Copyright (C) 2019-2026, Joshua Hertz, Giovanni Assad, Nikhen Nyo
< info@gruepr.com >

---------------
SPONSORS:

     Free code signing on Windows provided by SignPath.io, certificate by SignPath Foundation.

---------------
Description of gruepr:

     Gruepr is a program for splitting a section of 4-1000 students into optimized teams.
     It was inspired by CATME's team forming routine as described in their paper
     [ http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf ].

     Data about the students are collected and the students are split into teams of any desired size(s). A
     good distribution of students into teams can be based on:
        1) preventing isolated women, isolated men, isolated nonbinary persons, and/or single-gender teams;
        2) preventing isolated racial/ethnic minority students;
        3) achieving within each team either homogeneity or heterogeneity of up to 15 "attributes", which 
           could be skills self-assessments, work preferences, attitudes, major, age, GPA, or any 
           other question that is answered with a single numerical value or by selecting one or more 
           options from a limited set of possibilities;
        4) giving each team a unique project / topic based on expressed student preference;
        5) achieving a high degree of overlap in schedule freetime (with timezone awareness);
        6) separating particular students onto different teams;
        7) requiring particular students to be on the same team; and / or
        8) several more complicated applications of the above criteria.
        
     After the optimization process runs for some time, the best set of teams found is shown on the screen.
     You can choose whether to save this teamset, adjust this teamset by rearranging teams or students, or 
     change the teaming options and try again. The teamset can be saved in text, pdf, or spreadsheet format.

     The student data are typically collected using a survey that the students fill out. Gruepr will help
     the instructor create this survey, outputting the survey as either text files, a Canvas quiz, or, more
     commonly, a Google Form on the instructor's Google Drive. After collecting the students' survey
     responses, the results are loaded into gruepr. If using the Google Form or Canvas quiz, the results can
     be directly imported. If an alternate surveying instrument is used, the results can be a comma-
     separated-values (.csv) file, a tab-delimited-values (.txt) file, or an Excel (.xlsx) file with each
     question as a separate column and each student as a separate row.

     Integration with various learning management systems is currently in development. Canvas integrations
     currently use a user-generated API token. Gruepr can create the survey as an ungraded quiz in the
     Canvas course, can directly import the survey results, and can upload the created teams as groups in
     the Canvas course.

     COMPILING NOTES: Need C++20 and OpenMP on all systems. Enabling the speed optimization switch -O2 seems
     to offer significant speed boost; -O3 does not seem to offer any improvement.

---------------
Details on how the teams are optimized:

     To optimize the teams, a memetic (modified genetic) algorithm is used. First, a large population (10's
     of thousands) of random teamings (each is a "genome") is created and then refined over multiple 
     generations. In each generation, a small number of the highest scoring "elite" genomes are directly 
     copied (cloned) into the next generation, and the rest are created by mating tournament-selected parents
     using ordered crossover.
     
     The memetic component is that a local search is used to improve a selection of genomes each generation.
     A logarithmically spaced set of genomes undergo this process. It is an expensive process, so only 1% of
     the population is processed, and they are spaced more densely near the higher scoring genomes so that
     the time isn't wasted on genomes unlikely to be selected for mating the next generation. The process
     takes the most broken gene in the genome (the lowest-scoring negative team) and performs an exhaustive
     search swapping each teammate on that team with every other student on every other team, stopping once
     the genome is less broken (the number of negative teams decreases). If a genome has no negative teams,
     this process is skipped EXCEPT for the elite genomes. The elite genomes always go through an exhaustive
     search, either to fix a broken gene as described previously or, if no genes are broken, to neverthless
     try swapping each teammate on the lowest scoring team with every other student and keeping the swap if
     the genome's score increases, hill climbing by continuing with every possible swap and keeping each one
     that causes the overall genome score to rise.

     A genome's net score is the harmonic mean of the score for each team. Harmonic mean is used so that low
     scoring teams have more weight. Evolution proceeds for at least minGenerations and at most
     maxGenerations, displaying generation number and the score of that generation's best genome. Evolution
     stops (user can choose to keep it going) when the best score has remained +/- 1% for
     generationsOfStability generations or when maxGenerations is reached.


---------------
A Note about genetic algorithm efficiency:

     Genomes store a partitioning of students into teams as an array of ID numbers. For example, if teams are
     of size 4, a genome that starts [1 5 18 9 3 89 42 43 ...] has a first team with students 1, 5, 18, 9 and
     a second team with students 3, 89, 42, 43. There is unfortunate redundancy in this storage scheme, since
     a genome that starts [1 5 18 9 x x x x ...] and another with [x x x x 9 5 1 18...] both encode an
     identical team in two different ways.
     Since every genome has teams split at the same locations in the array, the ordered crossover isn't so
     bad a method for creating children since genomes are split at the team boundaries. Good parents create
     good children by passing on what's most likely good about their genome--good team(s). If the crossover
     were blind to the teammate boundaries, it would be less efficient, potentially even splitting up a good
     team if the crossover occurred in the middle of a preferred team. Good parents would more likely lead to
     good children if either: 1) the crossover split ocurred in the middle of a bad team (helpful), 2) the
     crossover split ocurred at a team boundary (helpful, but unlikely), or 3) the crossover split a good
     team but other parent has exact same good team in exact same location of genome (unhelpful--leads to
     preference for a single good genome and thus premature selection). Splitting always along team
     boundaries ensures primarily the second option happens, and thus good parents pass along good teams, in
     general, wherever they occur along the genome. However, there still are redundancies inherent in this
     encoding scheme, making it less efficient. Swapping the positions of two teammates within a team or of
     two whole teams within the list is represented by two different genomes. Additional inefficiencies are
     suggested by the Genetic Grouping Algorithm (GGA).
     
     Results from GA tuning investigation (2026-08-09):
          Effective (tried, improved):
               - Tuned the tournamentSize from 100 to 30
          Not promising (tried, ruled out):
               - Tuning numElites / adding non-mutated SuperElites — effect is noise-level.
               - Annealing/scheduling tournamentSize down over a run — actively worse than just holding a
                 good flat value; early diversity loss doesn't recover.
               - Island genepools with migration — neutral at every size tested (2-6 islands); adds real
                 complexity for no measurable gain.
          Note: a synthetic worst-case (a class of team-sized number of copies of identical students, teaming
                options favoring similarity, i.e., a well-defined optimal case of each team consisting of one
                set of identical copies) still isn't solved by anything above (though real data doesn't look
                like this).
          Promising / worth exploring:
               - To get around the redundancy-of-genome issue, sort indexes w/in each team and then each
                 team w/in the genome.

     Results from continued GA investigation (2026-08-20):
          Very effective memetic addition to the genetic algorithm:
               - Use exhaustive search to fix a broken gene (a negative scoring team) within a genome
               - Use exhaustive search to improve the lowest scoring team in an elite genome once it hss
                 no broken genes


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

     This software incorporates code from the open source Qt libraries, using version 6.9. These can be
     freely downloaded from < http://qt.io/download >.

     This software incorporates code from the open source QXlsx library (MIT License), used for reading
     and writing Excel (.xlsx) files. It can be freely downloaded from < https://github.com/QtExcel/QXlsx >.

     Icons and graphics are original creations for the gruepr project by
     Scout < https://scout.camd.northeastern.edu/ >.

     Several embedded fonts are used:
     Oxygen Mono, (C) 2012 Vernon Adams (vern@newtypography.co.uk);
     DM Sans (C) 2014-2017 Indian Type Foundry (info@indiantypefoundry.com); and
     Paytone One (C) 2011 The Paytone Project Authors (https://github.com/googlefonts/paytoneFont).
     All fonts are licensed according to the SIL OPEN FONT LICENSE Version 1.1; 

     Free code signing on Windows provided by SignPath.io, certificate by SignPath Foundation.
