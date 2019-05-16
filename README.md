gruepr

Copyright (C) 2019, Joshua Hertz < j.hertz@neu.edu >

---------------
Description of gruepr:

        Gruepr is a program for splitting a section of 4-200 students into optimized teams.
        It is originally based on CATME's team forming routine as described in their paper
        [ http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf ].

        All the student data is read from a file, and the students are split into teams of any desired size(s). A
        good distribution of students into teams is determined by a numerical score. The score can be based on:
           1) preventing isolated women, isolated men, and/or single-gender teams;
           2) preventing isolated URM students;
           3) achieving within each team either homogeneity or heterogeneity of up to 9 numerical "attribute
              levels", which could be skills assessments, work preferences, or attitudes;
           4) achieving a high degree of overlap in schedule freetime;
           5) preventing any particular students from being on the same team; and/or
           6) requiring any particular students to be on the same team.

        After optimizing the teams for some time, the best set of teams found is shown on the screen. The teams
        are displayed showing the students names, emails, gender, URM statusm and attribute levels. Each team's
        score is also shown along with a table of student availability at each time slot throughout the week. You
        can choose whether to save this teamset, adjust this teamset by swapping pairs of students, or throw away
        the teamset entirely and start over. If you save the teamset, three files can be saved: 1) an instructor's
        file containing all the team and student information; 2) a student's file showing the same but without
        listing team scores or student demographics/attributes; and 3) the section, team, and student names in a
        spreadsheet format.

        The datafile of student information has a specific required format. Using the Google Form allows you to
        download the data into exactly this file format without modification. After collecting responses, go to the
        Google Form and, in the "Responses" tab, click the three-dot icon and select "Download responses (.csv)".
        The gender question can be removed, if desired. The attribute questions on the Google Form (Section 3 of
        the form) can be modified, including reordering, adding, or removing questions. There can be anywhere
        between 0 to 9 of these attribute questions. The only requirement is that the responses that the students
        select MUST include one single digit number to indicate a quantitative measure of the answer to the question.
        The schedule section can be removed, or the numbers and names of the days and times can be altered. The only
        requirement here is to have 7 or fewer days and 24 or fewer hours in the matrix. A section question can be
        included so that multiple sections can all be sent the same form; if more than one section is present in the
        data, you can select which section to team. Additional questions for your own use can be added to the end of
        the form.

        The datafile must be a comma-separated-values (.csv) file with these contents:
           - header row, contains only the comma-separated question texts
           - each student on a separate row, starting at the row immediately after the header
           - in each row, comma-separated values are, in order:
               ~ timestamp
               ~ first name or preferred name
               ~ last name
               ~ email
               ~ [optional] "Woman", "Man", or any number of additional gender categories ("Prefer not to say",
                     "Non-binary", etc.)
               - [optional] "yes" or "no", to indicate whether the student comes from an underrepresented minority
                     group
               ~ [0 to 9 values] text responses to "attribute" questions, each in own field; first numeric digit
                     found in the text is the value used by the program
               ~ [0 to 7 values] semicolon-separated list of times each day that the student is unavailable to work
               ~ [optional] section
               ~ [optional] any additional notes for student (all remaining columns will get swallowed into this
                     field)

        COMPILING NOTES: Need C++11 compatibility. Using gcc/mingw, enabling the highest speed optimizations
        with -O3 seems to offer significant speed boost.


---------------
Details on how the teams are optimized:

        To optimize the teams, a genetic algorithm is used. First, a large population of 30,000 random teamings
        (each is a "genome") is created and then refined over multiple generations. In each generation, a small
        number of the highest scoring "elite" genomes are directly copied (cloned) into the next generation, and
        the rest are created by mating tournament-selected parents using ordered crossover. Once the next
        generation's genepool is created, each genome has 1 or more potential mutations, which is a swapping of
        two random locations on the genome.

        A genome's net score is the harmonic mean of the score for each team. Harmonic mean is used so that low
        scoring teams have more weight. Evolution proceeds for at least minGenerations and at most maxGenerations,
        displaying generation number and the score of that generation's best genome. Evolution stops (user can
        choose to keep it going) when the best score has remained +/- 1% for generationsOfStability generations or
        when maxGenerations is reached.


---------------
A Note about genetic algorithm efficiency:

        Unfortunately, there is redundancy in the array-based permutation-of-teammate-ID way that the teammates are
        encoded into the genome. For example, if teams are of size 4, one genome that starts [1 5 18 9 x x x x ...]
        and another that has [x x x x 9 5 1 18...] are encoding an identical team in two ways. Since every genome
        has teams split at the same locations in the array, the ordered crossover isn't so bad a method for creating
        children since genomes are split at the team boundaries. Good parents create good children by passing on
        what's most likely good about their genome--good team(s). If the crossover were blind to the teammate
        boundaries, it would be less efficient, potentially even splitting up a good team if the crossover occurred
        in the middle of a preferred team. Good parents would more likely lead to good children if either: 1) the
        crossover split ocurred in the middle of a bad team (helpful), 2) the crossover split ocurred at a team
        boundary (helpful, but unlikely), or 3) the crossover split a good team but other parent has exact same good
        team in exact same location of genome (unhelpful--leads to preference for a single good genome and thus
        premature selection). Splitting always along team boundaries ensures primarily the second option happens,
        and thus good parents pass along good teams, in general, wherever they occur along the genome. However,
        there still are redundancies inherent in this encoding scheme, making it less efficient. Swapping the
        positions of two teammates within a team or of two whole teams within the list is represented by two
        different genomes. Additional inefficiencies are suggested by the Genetic Grouping Algorithm (GGA).


---------------
DISCLAIMER:

        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
        Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
        option) any later version.

        This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
        implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
        for more details.

        You should have received a copy of the GNU General Public License along with this program.  If not, see
        < https://www.gnu.org/licenses/ >.

        This software incorporates code from the open source Qt libraries, using version 5.12.1. These can be freely
        downloaded from < http://qt.io/download >.

        Icons were originally created by Icons8 < https://icons8.com >. These icons have been made available under
        the creative commons license: Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).

        An embedded font is used: Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk),
        released under the SIL OPEN FONT LICENSE Version 1.1.
