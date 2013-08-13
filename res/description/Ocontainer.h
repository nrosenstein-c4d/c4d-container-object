
#ifndef Ocontainer_H
#define Ocontainer_H

    enum {
        Ocontainer = 1029700,

        // VERSION 0

        OCONTAINER_G_ACTIONS = 2012, // VERSION 1000
            OCONTAINER_PACKUP = 2001,
            OCONTAINER_UNPACK = 2002,
            OCONTAINER_NHIDDENCHILDREN = 2003,

            OCONTAINER_HIDETAGS = 2004,
            OCONTAINER_SHOWTAGS = 2005,
            OCONTAINER_NHIDDENTAGS = 2006,

        OCONTAINER_G_ICON = 2007,
            OCONTAINER_LOADCUSTOMICON = 2008,
            OCONTAINER_CLEARCUSTOMICON = 2009,
            OCONTAINER_HINTCOLOR = 2010,
            OCONTAINER_HINTTHICKNESS = 2011,
    };

#endif // Ocontainer_H

