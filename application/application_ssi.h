#ifndef APPLICATION_SSI_H
#define APPLICATION_SSI_H

/*List of SSI tags used in html files
  Notes:-
    1. This list is used to create a matching enum and string constant for each SSI tag
    2. Only append to end of list.  Do not insert, delete or reorder the existing items!
    3. Related items are assumed to be in sequence in the code e.g. days of week
    4. Use a backslash on every line except the last line when defining the tags
*/
#define APPLICATION_SSI_TAGS \
    x(hello)   \
    x(world)   \
    x(lpin)    \
    x(lrgbw)   \
    x(lnum)    \
    x(clpat)   \
    x(cltran)  \
    x(clreq)
      
    u16_t application_ssi_handler(int iIndex, char *pcInsert, int iInsertLen, void *connection_state);

    #endif