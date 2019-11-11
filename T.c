#include <string.h>
#define LENGHT_ARR 16000

const unsigned int cT[LENGHT_ARR] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

char quixote[] = "CHAPTER I.\n"
                 "WHICH TREATS OF THE CHARACTER AND "
                 "PURSUITS OF THE FAMOUS GENTLEMAN"
                 "DON QUIXOTE OF LA MANCHA \n"
                 "In a village of La Mancha, the name of which "
                 "I have no desire to call to mind, there lived not "
                 "long since one of those gentlemen that keep a "
                 "lance in the lance-rack, an old buckler, a lean "
                 "hack, and a greyhound for coursing. An olla of "
                 "rather more beef than mutton, a salad on most "
                 "nights, scraps on Saturdays, lentils on Fridays, "
                 "and a pigeon or so extra on Sundays, made "
                 "away with three-quarters of his income. The "
                 "rest of it went in a doublet of fine cloth and "
                 "velvet breeches and shoes to match for holi-"
                 "days, while on week-days he made a brave fi-"
                 "gure in his best homespun. He had in his house"
                 "a housekeeper past forty, a niece under twenty,"
                 "and a lad for the field and market-place, who"
                 "used to saddle the hack as well as handle the"
                 "bill-hook. The age of this gentleman of ours"
                 "was bordering on fifty; he was of a hardy habit,"
                 "spare, gaunt-featured, a very early riser and a"
                 "great sportsman. They will have it his surname"
                 "was Quixada or Quesada (for here there is so-"
                 "me difference of opinion among the authors"
                 "who write on the subject), although from rea-"
                 "sonable conjectures it seems plain that he was"
                 "called Quexana. This, however, is of but little"
                 "importance to our tale; it will be enough not to"
                 "stray a hair's breadth from the truth in the te-"
                 "lling of it. \n";

unsigned long int get_address_table(int index)
{
    if (index < LENGHT_ARR)
    {
        return (unsigned long int)(&cT[index]);
    }
    else
    {
        return (unsigned long int)(&cT[0]);
    }
}

unsigned long int get_address_quixote(int index)
{
    if (index < (int) strlen(quixote))
    {
        return (unsigned long int)(&quixote[index]);
    }
    else
    {
        return (unsigned long int)(&quixote[0]);
    }
}
