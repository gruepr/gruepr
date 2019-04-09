/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
// version 8.2
// 04/06/19
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019
// Joshua Hertz
// j.hertz@neu.edu
/////////////////////////////////////////////////////////////////////////////////////////////////////////

//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see < https://www.gnu.org/licenses/ >.

//    This software incorporates code from the open source Qt libraries,
//    using version 5.12.1. These can be freely downloaded from
//    < http://qt.io/download >.

//    Icons were created by or modified from < https://icons8.com >.
//    These icons have been made available under a creative commons license.

/////////////////////////////////////////////////////////////////////////////////////////////////////////

// Program for splitting a set of 4-200 students into optimized teams.
// Originally based on CATME's team forming routine as described in their paper:
// < http://advances.asee.org/wp-content/uploads/vol02/issue01/papers/aee-vol02-issue01-p09.pdf >

// All of the student data is read from a data file on the harddrive.
// The students are then split up into teams of the desired size.
// An optimized distribution of students into teams is determined by a "compatibility score."
// The compatability score can be based on:
//    1) preventing an isolated woman and/or isolated man,
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

// COMPILING NOTES: Need C++11 compatibility. Using gcc/mingw, enabling the highest speed optimizations with -O3 seems to offer significant speed boost.

// The optimization problem is very difficult (NP-hard? NP-complete?). There are, for example, almost 6E19 ways to partition 32
// students into 8 teams of 4. A genetic algorithm is used here to find a good set of teammates.
// First, each student is given an internal ID number from 0 to N-1. A large population of random teamings is then created.
// Each "genome" here represents one way to split the N students into n teams, and is represented as a permutation of the list {0, 2, 3...,N-1}.
// The population of genomes is then refined over multiple generations.
// From each generation a next generation is created.
// First, a small number of the highest scoring genomes (the "elites") are directly cloned into the next generation,
// then, the vast majority of the next generation are created by mating tournament-selected parents using order crossover (OX1).
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
//  -use parallel processing for faster operations--especially the breeding to next generation which can be completely done in parallel
//	-to get around redundancy of genome issue, store each genome as an unordered_set of unordered_sets. Each team is a set of IDs; each section is a set of teams.

/////////////////////////////////////////////////////////////////////////////////////////////////////////

// FUTURE WORK:
// - more fully account for non-binary gender
// - expand gender isolation penalty to prevent isolation along other personal attributes
// - improve appearance of adjust teammate dialog (instant feedback on swap results before "OK" adjusts or "Cancel" cancels the adjustment of the actual array)
// - if reached stability, continue with added mutation probability
// - change stability metric? (convergence = median or mean score relative to max)

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include <QApplication>
#include <QSplashScreen>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("gruepr");
    a.setApplicationName("gruepr");
    a.setApplicationVersion(GRUEPR_VERSION_NUMBER);

    gruepr w;

    QSplashScreen *splash = new QSplashScreen;
    QPixmap pic(":/icons/splash.png");
    splash->setPixmap(pic);
    splash->show();
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\nj.hertz@neu.edu", Qt::AlignCenter);
    QThread::sleep(4);
    w.show();
    splash->finish(&w);
    delete splash;

    return a.exec();
}
