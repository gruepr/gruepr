#ifndef GRUEPR_CONSTS
#define GRUEPR_CONSTS

#define TIMESTAMP_FORMAT1 "yyyy/MM/dd h:mm:ss AP"
#define TIMESTAMP_FORMAT2 "yyyy/MM/dd h:mm:ssAP"
#define TIMESTAMP_FORMAT3 "M/d/yyyy h:mm:ss"
#define TIMESTAMP_FORMAT4 "M/d/yyyy h:mm"

#define USER_REGISTRATION_URL "https://script.google.com/macros/s/AKfycbwqGejEAumqgwpxDdXrV5CJS54gm_0N_du7BweU3wHG-XORT8g/exec"

#include <Qt>
#include "GA.h"


const int MAX_ATTRIBUTES = 15;                          // maximum number of skills/attitudes
const int MAX_STUDENTS = MAX_RECORDS;                   // each student is a "record" in the genetic algorithm
const int MAX_IDS = 2 * MAX_STUDENTS;                   // since students can be removed and added yet IDs always increase, need more IDs than possible students
const int MAX_TEAMS = MAX_STUDENTS/2;
const int MAX_DAYS = 7;                                 // resolution of scheduling is 1 hr, and scope is weekly
const int MAX_BLOCKS_PER_DAY = 24;
const int MAX_NOTES_FIELDS = 99;                        // allowed number of notes fields in a survey

const int TEAMINFO_DISPLAY_ROLE = Qt::UserRole;         // data with this role is stored in each column of the team info display tree, shown when team is collapsed
const int TEAMINFO_SORT_ROLE = Qt::UserRole + 1;        // data with this role is stored in each column of the team info display tree, used when sorting the column
const int TEAM_NUMBER_ROLE = Qt::UserRole + 2;          // data with this role is stored in column 0 of the team info display tree, used when swapping teams or teammates

const int DIALOG_SPACER_ROWHEIGHT = 20;
const int LG_DLG_SIZE = 600;
const int SM_DLG_SIZE = 300;
const int XS_DLG_SIZE = 200;
const int SMALL_SCREENSIZE_WIN = 900;
const int SMALL_SCREENSIZE_MAC = 800;
const int REDUCED_ICON_SIZE = 30;

//map of the "meaning" of strings that might be used in the Google Form to refer to hours of the day
const char TIME_NAMES[] {"1am,1 am,1:00,2am,2 am,2:00,3am,3 am,3:00,4am,4 am,4:00,5am,5 am,5:00,6am,6 am,6:00,7am,7 am,7:00,8am,8 am,8:00,9am,9 am,9:00,10am,10 am,10:00,"
                         "11am,11 am,11:00,12pm,12 pm,12:00,1pm,1 pm,13:00,2pm,2 pm,14:00,3pm,3 pm,15:00,4pm,4 pm,16:00,5pm,5 pm,17:00,6pm,6 pm,18:00,7pm,7 pm,19:00,8pm,8 pm,20:00,"
                         "9pm,9 pm,21:00,10pm,10 pm,22:00,11pm,11 pm,23:00,12am,12 am,0:00,noon,midnight"};
const int TIME_MEANINGS[] {1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,12,
                           13,13,13,14,14,14,15,15,15,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,22,23,23,23,0,0,0,12,0};

const char TIMEZONENAMES[] {"International Date Line West [GMT-12:00];\"Samoa: Midway Island, Samoa [GMT-11:00]\";Hawaiian: Hawaii [GMT-10:00];Alaskan: Alaska [GMT-09:00];"
                            "Pacific: US and Canada, Tijuana [GMT-08:00];Mountain: US and Canada [GMT-07:00];\"Mexico Pacific: Chihuahua, La Paz, Mazatlan [GMT-07:00]\";"
                            "Central: US and Canada [GMT-06:00];Canada Central: Saskatchewan [GMT-06:00];\"Mexico Central: Guadalajara, Mexico City, Monterrey [GMT-06:00]\";"
                            "Central America: Central America [GMT-06:00];Eastern: US and Canada [GMT-05:00];\"S.A. Pacific: Bogota, Lima, Quito [GMT-05:00]\";"
                            "Atlantic: Canada [GMT-04:00];\"S.A. Western: Caracas, La Paz [GMT-04:00]\";Pacific S.A.: Santiago [GMT-04:00];Newfoundland and Labrador [GMT-03:30];"
                            "E. South America: Brasilia [GMT-03:00];\"S.A. Eastern: Buenos Aires, Georgetown [GMT-03:00]\";Greenland [GMT-03:00];Mid-Atlantic Islands [GMT-02:00];"
                            "Azores [GMT-01:00];Cape Verde [GMT-01:00];\"Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London [GMT +00:00]\";"
                            "\"Greenwich: Casablanca, Monrovia [GMT +00:00]\";\"Central Europe: Belgrade, Bratislava, Budapest, Ljubljana, Prague [GMT+01:00]\";"
                            "\"Central Europe: Sarajevo, Skopje, Warsaw, Zagreb [GMT+01:00]\";\"Romance: Brussels, Copenhagen, Madrid, Paris [GMT+01:00]\";"
                            "\"W. Europe: Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna [GMT+01:00]\";W. Central Africa: West Central Africa [GMT+01:00];"
                            "E. Europe: Bucharest [GMT+02:00];Egypt: Cairo [GMT+02:00];\"FLE: Helsinki, Kiev, Riga, Sofia, Tallinn, Vilnius [GMT+02:00]\";"
                            "\"GTB: Athens, Istanbul, Minsk [GMT+02:00]\";Israel: Jerusalem [GMT+02:00];\"South Africa: Harare, Pretoria [GMT+02:00]\";"
                            "\"Russian: Moscow, St. Petersburg, Volgograd [GMT+03:00]\";\"Arab: Kuwait, Riyadh [GMT+03:00]\";E. Africa: Nairobi [GMT+03:00];"
                            "Arabic: Baghdad [GMT+03:00];Iran: Tehran [GMT+03:30];\"Arabian: Abu Dhabi, Muscat [GMT+04:00]\";\"Caucasus: Baku, Tbilisi, Yerevan [GMT+04:00]\";"
                            "Transitional Islamic State of Afghanistan: Kabul [GMT+04:30];Ekaterinburg [GMT+05:00];\"West Asia: Islamabad, Karachi, Tashkent [GMT+05:00]\";"
                            "\"India: Chennai, Kolkata, Mumbai, New Delhi [GMT+05:30]\";Nepal: Kathmandu [GMT+05:45];\"Central Asia: Astana, Dhaka [GMT+06:00]\";"
                            "Sri Lanka: Sri Jayawardenepura [GMT+06:00];\"N. Central Asia: Almaty, Novosibirsk [GMT+06:00]\";Myanmar: Yangon Rangoon [GMT+06:30];"
                            "\"S.E. Asia: Bangkok, Hanoi, Jakarta [GMT+07:00]\";North Asia: Krasnoyarsk [GMT+07:00];\"China: Beijing, Chongqing, Hong Kong SAR, Urumqi [GMT+08:00]\";"
                            "\"Singapore: Kuala Lumpur, Singapore [GMT+08:00]\";Taipei: Taipei [GMT+08:00];W. Australia: Perth [GMT+08:00];"
                            "\"North Asia East: Irkutsk, Ulaanbaatar [GMT+08:00]\";Korea: Seoul [GMT+09:00];\"Tokyo: Osaka, Sapporo, Tokyo [GMT+09:00]\";"
                            "Yakutsk: Yakutsk [GMT+09:00];A.U.S. Central: Darwin [GMT+09:30];Cen. Australia: Adelaide [GMT+09:30];"
                            "\"A.U.S. Eastern: Canberra, Melbourne, Sydney [GMT+10:00]\";E. Australia: Brisbane [GMT+10:00];Tasmania: Hobart [GMT+10:00];"
                            "Vladivostok: Vladivostok [GMT+10:00];\"West Pacific: Guam, Port Moresby [GMT+10:00]\";"
                            "\"Central Pacific: Magadan, Solomon Islands, New Caledonia [GMT+11:00]\";\"Fiji Islands: Fiji Islands, Kamchatka, Marshall Islands [GMT+12:00]\";"
                            "\"New Zealand: Auckland, Wellington [GMT+12:00]\";Tonga: Nuku'alofa [GMT+13:00]"};

// Options for the team names. A name for each list of names must be given.
// If name ends with period, names are numeric and increase without end
// If name ends with asterisk, names beyond list get repeated (as in A, B, C... -> AA, BB, CC... -> AAA, BBB, CCC...)
// If name ends with tilde, names beyond list get repeated with space between (as in A, B, C... -> A A, B B, C C... -> A A A, B B B, C C C...)
// If name ends with hashtag, names beyond list get 'sequeled' (as in A, B, C... -> A 2, B 2, C 2... -> A 3, B 3, C 3...)
const char TEAMNAMECATEGORIES[] {"Arabic numbers.,"
                                 "Roman numerals.,"
                                 "Hexadecimal numbers.,"
                                 "Binary numbers.,"
                                 "English letters*,"
                                 "Greek letters (uppercase)*,"
                                 "Greek letters (lowercase)*,"
                                 "NATO phonetic alphabet~,"
                                 "Chemical elements#,"
                                 "Papal names#,"
                                 "Constellations#,"
                                 "Crayola crayon colors#,"
                                 "Genres of music#,"
                                 "Cheeses#,"
                                 "Shakespeare plays (RSC chron.)#,"
                                 "All time best-selling albums in US#,"
                                 "Minor Simpsons characters#,"
                                 "Bones of the human skeleton (rand.)@,"
                                 "Minor league baseball teams#,"
                                 "Discontinued Olympic sports#,"
                                 "Groups of animals#"};
const char TEAMNAMELISTS[]   {";"
                              ";"
                              ";"
                              ";"
                              "A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z;"
                              "Α,Β,Γ,Δ,Ε,Ζ,Η,Θ,Ι,Κ,Λ,Μ,Ν,Ξ,Ο,Π,Ρ,Σ,Τ,ϒ,Φ,Χ,Ψ,Ω;"
                              "α,β,γ,δ,ε,ζ,η,θ,ι,κ,λ,μ,ν,ξ,ο,π,ρ,σ,τ,υ,φ,χ,ψ,ω;"
                              "Alfa,Bravo,Charlie,Delta,Echo,Foxtrot,Golf,Hotel,India,Juliett,Kilo,Lima,Mike,"
                                 "November,Oscar,Papa,Quebec,Romeo,Sierra,Tango,Uniform,Victor,Whiskey,X-ray,Yankee,Zulu;"
                              "Hydrogen,Helium,Lithium,Beryllium,Boron,Carbon,Nitrogen,Oxygen,Fluorine,Neon,Sodium,Magnesium,"
                                 "Aluminum,Silicon,Phosphorus,Sulfur,Chlorine,Argon,Potassium,Calcium,Scandium,Titanium,Vanadium,"
                                 "Chromium,Manganese,Iron,Cobalt,Nickel,Copper,Zinc,Gallium,Germanium,Arsenic,Selenium,Bromine,Krypton,"
                                 "Rubidium,Strontium,Yttrium,Zirconium,Niobium,Molybdenum,Technetium,Ruthenium,Rhodium,Palladium,Silver,"
                                 "Cadmium,Indium,Tin,Antimony,Tellurium,Iodine,Xenon,Cesium,Barium,Lanthanum,Cerium,Praseodymium,Neodymium,"
                                 "Promethium,Samarium,Europium,Gadolinium,Terbium,Dysprosium,Holmium,Erbium,Thulium,Ytterbium,Lutetium,"
                                 "Hafnium,Tantalum,Tungsten,Rhenium,Osmium,Iridium,Platinum,Gold,Mercury,Thallium,Lead,Bismuth,Polonium,"
                                 "Astatine,Radon,Francium,Radium,Actinium,Thorium,Protactinium,Uranium,Neptunium,Plutonium,Americium,Curium,"
                                 "Berkelium,Californium,Einsteinium,Fermium,Mendelevium,Nobelium,Lawrencium,Rutherfordium,Dubnium,Seaborgium,"
                                 "Bohrium,Hassium,Meitnerium,Darmstadtium,Roentgenium,Copernicium,Nihonium,Flerovium,Moscovium,Livermorium,"
                                 "Tennessine,Oganesson;"
                              "Adrian,Benedict,Clement,Damasus,Eugene,Felix,Gregory,Hilarius,Innocent,John,Leo,Martin,Nicholas,Pius,Romanus,"
                                 "Stephen,Theodore,Urban,Victor,Zosimus,Alexander,Boniface,Celestine,Dionysius,Evaristus,Fabian,Gelasius,Honorius,"
                                 "Julius,Lucius,Marcellus,Pelagius,Sixtus,Telesphorus,Valentine,Zephyrinus;"
                              "Andromeda,Bootes,Cassiopeia,Draco,Equuleus,Fornax,Gemini,Hydra,Indus,Leo,Musca,Norma,Orion,Perseus,Reticulum,"
                                 "Sagittarius,Taurus,Ursa Major,Virgo;"
                              "Aquamarine,Burnt Sienna,Chartreuse,Dandelion,Emerald,Fuchsia,Goldenrod,Heat Wave,Indigo,Jungle Green,Laser Lemon,Mulberry,"
                                 "Neon Carrot,Orchid,Periwinkle,Razzmatazz,Scarlet,Thistle,Ultra Orange,Vivid Tangerine,Wisteria,Yosemite Campfire,Zircon;"
                              "Acapella,Blues,Country,Doo-Wop,EDM,Fado,Gospel,Hip-Hop,Indie,Jazz,K-Pop,Lullaby,Mariachi,New Age,Opera,Punk,"
                                 "Qawwali,Reggae,Soundtrack,Tejano,Underground,Vocal,Western Swing,Xhosa,Yodeling,Zydeco;"
                              "Asiago,Brie,Cheddar,Derby,Edam,Feta,Gouda,Havarti,Iberico,Jarlsberg,Kaseri,Limburger,Manchego,Neufchatel,Orla,"
                                 "Paneer,Queso Fresco,Ricotta,Siraz,Tyn Grug,Ulloa,Vignotte,Weichkaese,Xynotyro,Yorkshire Blue,Zamorano;"
                              "Taming of the Shrew,Henry VI,Two Gentlemen of Verona,Titus Andronicus,Richard III,Comedy of Errors,"
                                 "Love's Labour's Lost,Midsummer Night's Dream,Romeo and Juliet,Richard II,King John,Merchant of Venice,"
                                 "Henry IV,Much Ado about Nothing,Henry V,As You Like It,Julius Caesar,Hamlet,Merry Wives of Windsor,"
                                 "Twelfth Night,Troilus and Cressida,Othello,Measure for Measure,All's Well That Ends Well,Timon of Athens,"
                                 "King Lear,Macbeth,Antony and Cleopatra,Coriolanus,Pericles,Cymbeline,Winter's Tale,Tempest,Henry VIII,"
                                 "Two Noble Kinsmen;"
                              "Thriller,Eagles' Greatest Hits,Come On Over,Rumours,The Bodyguard,Back In Black,The Dark Side of the Moon,"
                                 "Saturday Night Fever,Bat Out of Hell,Bad,Led Zeppelin IV,21,Jagged Little Pill,1,ABBA's Greatest Hits,"
                                 "Appetite for Destruction,Hotel California,Supernatural,Metallica,The Immaculate Collection,Falling Into You,"
                                 "Born in the U.S.A.,Let's Talk About Love,The Wall,Sgt. Pepper's Lonely Hearts Club Band,Titanic,"
                                 "Dirty Dancing,Brothers in Arms,Nevermind,Dangerous,Abbey Road,Grease,Goodbye Yellow Brick Road,"
                                 "Slippery When Wet!,Music Box,Hybrid Theory,The Eminem Show,Come Away With Me,Unplugged,...Baby One More Time,Legend,"
                                 "Tapestry,No Jacket Required,Queen's Greatest Hits,True Blue,Bridge Over Troubled Water,The Joshua Tree,Purple Rain,"
                                 "Faith,Elvis' Christmas Album;"
                              "Artie Ziff,Brunella Pommelhorst,Cleetus,Disco Stu,Edna Krabappel,Frank 'Grimy' Grimes,Ginger Flanders,"
                                 "Helen Lovejoy,Itchy,Jebediah Springfield,Kent Brockman,Luann Van Houten,Mayor Quimby,Ned Flanders,Professor Frink,"
                                 "Queen Helvetica,Ruth Powers,Sideshow Bob,Troy McClure,Uter Zorker,Waylon Smithers,Xoxchitla,Yes Guy,Zelda;"
                              "Femur,Patella,Mandible,Hip,Metacarpal,Ulna,Humerus,Lacrimal,Distal Phalange,Cuboid,Trapezium,Coccyx,Zygomatic,Sternum,"
                                 "Ethmoid,Pisiform,Maxiallary,Lumbar Vertebrae,Stapes,Scapula,Navicular,Hamate,Rib,Hyoid,Occipital,Talus,Malleus,"
                                 "Triquetrum,Incus,Clavicle,Fibula,Proximal Phalange,Tibia,Lunate,Frontal,Palatine,Parietal,Medial Cuneiform,Vomer,"
                                 "Thoracic Vertebrae,Nasal,Capitate,Inferior Nasal Concha,Scaphoid,Sacrum,Temporal,Middle Cuneiform,Sphenoid,Calcaneus,"
                                 "Lateral Cuneiform,Radius,Cervical Vertebrae,Trapezoid;"
                              "Aviators,Blue Rocks,Crawdads,Drillers,Emeralds,Fireflies,GreenJackets,Hammerheads,Isotopes,Jumbo Shrimp,"
                                 "Knights,Lugnuts,Mighty Mussels,Nuts,Pelicans,Quakes,Rumble Ponies,Skeeters,Trash Pandas,Wood Ducks,Yard Goats,"
                                 "AquaSox,Bats,Curve,Drive,Express,Flying Squirrels,Grasshoppers,Hops,IronPigs,Blue Jays,Kernels,Lookouts,"
                                 "Mudcats,Naturals,Patriots,RubberDucks,Sod Poodles,TinCaps,Whitecaps,Baysox,Hot Rods,Marauders,SeaWolves,"
                                 "Cyclones,Bisons,RiverDogs,Threshers,Clippers,Hooks,Dragons,Tortugas,Shorebirds,Bulls,Chihuahuas,Woodpeckers,"
                                 "IronBirds,Nationals,Grizzlies,RoughRiders,Stripers,Senators,Snappers,Shuckers,Hillcats,Redbirds,RockHounds,"
                                 "Renegades,66ers,Cubs,BlueClaws,Cannon Ballers,Captains,Storm,Flying Tigers,Loons,Travelers,Tourists,Biscuits,"
                                 "Sounds,Fisher Cats,Tides,Barons,Dodgers,Storm Chasers,Cardinals,Blue Wahoos,Sea Dogs,River Bandits,Fightin Phils,"
                                 "Aces,Red Wings,River Cats,Red Sox,Lake Bees,Missions,Giants,RailRiders,Mets,Saints,Ports,Rainiers,Tarpons,Smokies,"
                                 "Mud Hens,Dust Devils,Canadians,Rawhide,Wind Surge,Dash,Timber Rattlers;"
                              "Angling,Bowling,Cannon Shooting,Dog Sledding,Engraving,Fire Fighting,Gliding,Hurling,India Club Swinging,Jeu de Paume,"
                                 "Korfball,Lacrosse,Motorcycle Racing,Orchestra,Pigeon Racing,Roller Hockey,Savate,Tug of War,Vaulting,Waterskiing,"
                                 "Ballooning,Croquet,Dueling Pistol,Kaatsen,Life Saving,Plunge Distance Diving,Rope Climb,Solo Synchronized Swimming,"
                                 "Standing High Jump,Polo;"
                              "Troop of Apes,Swarm of Bees,Murder of Crows,Drove of Donkeys,Convocation of Eagles,School of Fish,Gaggle of Geese,"
                                 "Cackle of Hyenas,Mess of Iguanas,Shadow of Jaguars,Mob of Kangaroos,Pride of Lions,Cartload of Monkeys,"
                                 "Blessing of Narwhals,Parliament of Owls,Pod of Porpoises,Bevy of Quail,Unkindness of Ravens,Shiver of Sharks,"
                                 "Rafter of Turkeys,Glory of Unicorns,Nest of Vipers,Confusion of Wildebeest,Herd of Yaks,Dazzle of Zebras,Army of Ants,"
                                 "Fluffle of Bunnies,Brood of Chickens,Dule of Doves,Gang of Elk,Skulk of Foxes,Cloud of Gnats,Bloat of Hippopotamuses,"
                                 "Smuck of Jellyfish,Litter of Kittens,Plague of Locusts,Labor of Moles,Watch of Nightingales,Bed of Oysters,"
                                 "String of Ponies,Flock of Quetzals,Crash of Rhinos,Dray of Squirrels,Bale of Turtles,Herd of Urchin,"
                                 "Committee of Vultures,Colony of Weasels"};

#endif // GRUEPR_CONSTS
