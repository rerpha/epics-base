/*
 *      $Id$
 *
 * 	File descriptor management C++ class library
 * 	(for multiplexing IO in a single threaded environment)
 *
 *      Author  Jeffrey O. Hill
 *              johill@lanl.gov
 *              505 665 1831
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 *
 * History
 * $Log$
 * Revision 1.5  1997/04/23 17:22:58  jhill
 * fixed WIN32 DLL symbol exports
 *
 * Revision 1.4  1997/04/10 19:45:38  jhill
 * API changes and include with  not <>
 *
 * Revision 1.3  1996/11/02 02:04:41  jhill
 * fixed several subtle bugs
 *
 * Revision 1.2  1996/09/04 21:50:16  jhill
 * added hashed fd to fdi convert
 *
 * Revision 1.1  1996/08/13 22:48:21  jhill
 * dfMgr =>fdManager
 *
 *
 */

#ifndef fdManagerH_included
#define fdManagerH_included

#include <stdio.h>

#include "tsDLList.h"
#include "resourceLib.h"
#include "osiTime.h"
#include "osiSock.h"

enum fdRegType {fdrRead, fdrWrite, fdrExcp, fdRegTypeNElem};
enum fdRegState {fdrActive, fdrPending, fdrLimbo};

class epicsShareClass fdRegId  
{
public:
	fdRegId (const SOCKET fdIn, const fdRegType typeIn) :
		fd(fdIn), type(typeIn) {}

	SOCKET getFD ()
	{
		return this->fd;
	}

	fdRegType getType ()
	{
		return this->type;
	}

        int operator == (const fdRegId &idIn)
        {
                return this->fd == idIn.fd && this->type==idIn.type;
        }

        resTableIndex resourceHash (unsigned nBitsId) const
        {
                unsigned        src = (unsigned) this->fd;
                resTableIndex   hashid;
 
                hashid = src;
                src = src >> nBitsId;
                while (src) {
                        hashid = hashid ^ src;
                        src = src >> nBitsId;
                }
		hashid = hashid ^ this->type;

                //
                // the result here is always masked to the
                // proper size after it is returned to the resource class
                //
                return hashid;
        }

	virtual void show (unsigned level) const;
private:
        const SOCKET	fd;
	const fdRegType	type;
};


//
// fdReg
// file descriptor registration
//
class fdReg : public tsDLNode<fdReg>, public fdRegId, public tsSLNode<fdReg> {
        friend class fdManager;
public:
	epicsShareFunc inline fdReg (const SOCKET fdIn, const fdRegType typ, 
			const unsigned onceOnly=0);
	epicsShareFunc virtual ~fdReg ();

	epicsShareFunc virtual void show(unsigned level) const;
	
	//
	// Called by the file descriptor manager:
	// 1) If the fdManager is deleted and there are still 
	// fdReg objects attached
	// 2) Immediately after calling "callBack()" if
	// the constructor specified "onceOnly" 
	//
	// fdReg::destroy() does a "delete this"
	//
	epicsShareFunc virtual void destroy ();
private:

        //
        // called when there is activity on the fd
	// NOTES
	// 1) the fdManager will call this only once during the
	// lifetime of a fdReg object if the constructor
	// specified "onceOnly"
        //
        epicsShareFunc virtual void callBack ()=0;

	unsigned char 	state; // fdRegState goes here
	unsigned char	onceOnly;
};

//
// fdManager
// file descriptor manager
//
class fdManager {
friend class fdReg;
public:
        epicsShareFunc fdManager();
        epicsShareFunc ~fdManager();
        epicsShareFunc void process (const osiTime &delay);

	//
	// returns NULL if the fd is unknown
	//
	epicsShareFunc fdReg *lookUpFD(const int fd, const fdRegType type);
private:
        tsDLList<fdReg>	regList;
        tsDLList<fdReg>	activeList;
	resTable<fdReg,fdRegId> fdTbl;
	fd_set		fdSets[fdRegTypeNElem];

        int             maxFD;
	unsigned	processInProg;
	//
	// Set to fdreg when in call back
	// and nill ortherwise
	//
	fdReg		*pCBReg; 

	void installReg (fdReg &reg);
        void removeReg (fdReg &reg);
};

epicsShareExtern fdManager fileDescriptorManager;

//
// fdManagerMaxInt ()
//
inline int fdManagerMaxInt (int a, int b)
{
        if (a>b) {
                return a;
        }
        else {
                return b;
        }
}
 
//
// fdReg::fdReg()
//
inline fdReg::fdReg (const SOCKET fdIn, const fdRegType typIn, 
		const unsigned onceOnlyIn) : 
	fdRegId(fdIn,typIn), state(fdrLimbo), onceOnly(onceOnlyIn)
{
	assert (fdIn>=0);
        if (!FD_IN_FDSET(fdIn)) {
		fprintf (stderr, "%s: fd > FD_SETSIZE ignored\n", 
			__FILE__);
		return;
	}
	fileDescriptorManager.installReg(*this);
}

#endif // fdManagerH_included
 
