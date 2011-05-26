// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif
#define FreeMapSector 		0
//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
   int result = ReadAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
   int result = WriteAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
	numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(buf + ( position - (firstSector * SectorSize) ), into, numBytes);
    delete [] buf;
    return numBytes;
}


BitMap* getFreeMap(){
	DEBUG('f', "opening free \n");
	OpenFile* freeMapFile = new OpenFile(FreeMapSector);
    BitMap* freeMap = new BitMap(NumSectors);
    DEBUG('f', "Fetching free Map\n");
    freeMap->FetchFrom(freeMapFile);
    return freeMap;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
  
#ifdef FILESYS
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;
    numSectors = fileLength/SectorSize;

    int total_length = fileLength - position + numBytes;
    int num_sectors = divRoundUp(total_length, SectorSize);
    int first_file_sector = hdr->ByteToSector(0); // Assume files start at beginning of sector

    if (first_file_sector + num_sectors > SectorSize*NumSectors) {
	return 0;
    }
    DEBUG('1', "Entering writeAt func...\n");

    // CASE 1
    if ((position + numBytes) <= fileLength) {
            //DEBUG('1', "Case 1!-------------------------\n");
	    if ((position + numBytes) > fileLength)
		numBytes = fileLength - position;

	    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
				numBytes, position, fileLength);

	    firstSector = divRoundDown(position, SectorSize);
	    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
	    numSectors = 1 + lastSector - firstSector;

	    buf = new char[numSectors * SectorSize];

	    firstAligned = (position == (firstSector * SectorSize));
	    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	    // read in first and last sector, if they are to be partially modified
	    if (!firstAligned)
		ReadAt(buf, SectorSize, firstSector * SectorSize);	
	    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
		ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
					SectorSize, lastSector * SectorSize);	

            //DEBUG('1', "Midway through....\n");

	    // copy in the bytes we want to change 
	    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

	    // write modified sectors back
	    for (i = firstSector; i <= lastSector; i++)	
		synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
						&buf[(i - firstSector) * SectorSize]);
	    delete [] buf;
	    //DEBUG('1', "Exiting case 1....\n");

	    //return numBytes;
    }

    // CASE 2
    else if (position < fileLength) {
	//DEBUG('1', "Case 2!-------------------------\n");
	//int last_sector = hdr->ByteToSector(fileLength - 1);
	int last_byte = position;
	int offset = last_byte - divRoundDown(last_byte, SectorSize)*SectorSize;
	int newSectors  = divRoundUp(numBytes - (SectorSize - offset), SectorSize);

	OpenFile* freeMapFile = new OpenFile(FreeMapSector);
        BitMap* freeMap = new BitMap(NumSectors);

        DEBUG('1', "Fetching free Map 88888888888888888888888888\n");
        freeMap->FetchFrom(freeMapFile);
	DEBUG('1', "DONE Fetching free Map 88888888888888888888888888\n");

	if (!hdr->ExtendFile(freeMap, newSectors)) {
		printf("Not enough free disk space to write to file. Ignoring write.\n");
		return 0;
	}

	freeMap->WriteBack(freeMapFile);

	delete freeMapFile;
	delete freeMap;

	//WriteAt(from, numBytes, position);

if ((position + numBytes) > fileLength) {
		numBytes = fileLength - position;
            }

	    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
				numBytes, position, fileLength);

	    firstSector = divRoundDown(position, SectorSize);
	    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
	    numSectors = 1 + lastSector - firstSector;

	    buf = new char[numSectors * SectorSize];

	    firstAligned = (position == (firstSector * SectorSize));
	    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	    // read in first and last sector, if they are to be partially modified
	    if (!firstAligned)
		ReadAt(buf, SectorSize, firstSector * SectorSize);	
	    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
		ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
					SectorSize, lastSector * SectorSize);	

	    // copy in the bytes we want to change 
	    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);


	    // write modified sectors back
	    for (i = firstSector; i <= lastSector; i++)	 {
		DEBUG('1', "Writing modified sector %d at location given %d\n", i, hdr->ByteToSector(i * SectorSize));
		synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
						&buf[(i - firstSector) * SectorSize]);
	    }
	    delete [] buf;
    }

    // CASE 3
    else if (position >= fileLength) {
        position = fileLength;

        //DEBUG('1', "Case 3!-------------------------\n");

	//int last_sector = hdr->ByteToSector(fileLength - 1);
	int last_byte;

	if (fileLength == 0) {
		last_byte = 0;
	}
	else {
		last_byte = fileLength - 1;
	}

	int offset = last_byte - divRoundDown(last_byte, SectorSize)*SectorSize;

	int newSectors  = divRoundUp(numBytes - (SectorSize - offset), SectorSize);

	DEBUG('1', "Currently have %d sectors and want %d new sectors\n", numSectors, newSectors);

	if ((numSectors == 0)&&(newSectors == 0)) {
		newSectors++;
	}
	

	DEBUG('1', "OPENING free Map 88888888888888888888888888\n");
	OpenFile* freeMapFile = new OpenFile(FreeMapSector);
	DEBUG('1', "DONE OPENING free Map 88888888888888888888888888\n");
        BitMap* freeMap = new BitMap(NumSectors);

        DEBUG('1', "Fetching free Map 88888888888888888888888888\n");
        freeMap->FetchFrom(freeMapFile);
	DEBUG('1', "DONE Fetching free Map 88888888888888888888888888\n");;

	
	if (!hdr->ExtendFile(freeMap, newSectors)) {
		printf("Not enough free disk space to write to file. Ignoring write.\n");
		return 0;
	}

	DEBUG('1', "STARTING FREE MAP WRITE BACK 9999999999999999999999999999999999999\n");;
	freeMap->WriteBack(freeMapFile);
	DEBUG('1', "DONE Writing Back free Map 9999999999999999999999999999999999999\n");

	delete freeMapFile;
	delete freeMap;

	fileLength = hdr->FileLength();

	//WriteAt(from, numBytes, position);

	    if ((position + numBytes) > fileLength) {
		numBytes = fileLength - position;
            }

	    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
				numBytes, position, fileLength);

	    firstSector = divRoundDown(position, SectorSize);
	    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
	    numSectors = 1 + lastSector - firstSector;

	    buf = new char[numSectors * SectorSize];

	    firstAligned = (position == (firstSector * SectorSize));
	    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	    // read in first and last sector, if they are to be partially modified
	    if (!firstAligned)
		ReadAt(buf, SectorSize, firstSector * SectorSize);	
	    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
		ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
					SectorSize, lastSector * SectorSize);	

	    // copy in the bytes we want to change 
	    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

	    // write modified sectors back
	    for (i = firstSector; i <= lastSector; i++)	 {
		DEBUG('1', "Writing modified sector %d at location given %d\n", i, hdr->ByteToSector(i * SectorSize));
		synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
						&buf[(i - firstSector) * SectorSize]);
	    }
	    delete [] buf;
    }
    //DEBUG('1', "Exiting writeAt func...\n");

    return numBytes;    
    
#elif defined FIXLESYS
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;
    if (numBytes <=0)
    	return 0;

    if (position+numBytes <= fileLength){ //if in bounds

		DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
				numBytes, position, fileLength);


		//LEFTOFF finding where  we can write in the sector block 
		//if not at start or crosses multiple sectors
		firstSector = divRoundDown(position, SectorSize);
		lastSector = divRoundDown(position + numBytes - 1, SectorSize);
		numSectors = 1 + lastSector - firstSector;

		buf = new char[numSectors * SectorSize];

		firstAligned = (position == (firstSector * SectorSize));
		lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	// read in first and last sector, if they are to be partially modified
		if (!firstAligned)
		    ReadAt(buf, SectorSize, firstSector * SectorSize);	
		if (!lastAligned && ((firstSector != lastSector) || firstAligned))
		    ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
					SectorSize, lastSector * SectorSize);	

	// copy in the bytes we want to change 
		bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

	// write modified sectors back
		for (i = firstSector; i <= lastSector; i++)	
		    synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
						&buf[(i - firstSector) * SectorSize]);
		delete [] buf;
		return numBytes;
		
    }//if in bounds
    else if( (position >= fileLength) ) { //Append end or past
    	//LEFTOFF
    	int start = fileLength; // we need to start adding sectors from end of the file
	firstSector = divRoundDown(start, SectorSize);
	lastSector = divRoundUp(position + numBytes - 1, SectorSize);
	numSectors = 1 + lastSector - firstSector;
	BitMap* freeMap = getFreeMap();    	
	if (hdr->ExtendFile(freeMap, numSectors) == -1){
		bool noSpace = false;
		ASSERT(noSpace);
	}

	buf = new char[numSectors * SectorSize];

	firstAligned = (position == (firstSector * SectorSize));
	lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	//TODO LEFTOF TEST
	
	// read in first and last sector, if they are to be partially modified
	if (!firstAligned)
	    ReadAt(buf, SectorSize, firstSector * SectorSize);	
	if (!lastAligned && ((firstSector != lastSector) || firstAligned))
	    ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

	// copy in the bytes we want to change 
	bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

	// write modified sectors back
	for (i = firstSector; i <= lastSector; i++)	
	    synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
	delete [] buf;
	return numBytes;
    	
    }
    else if(position+numBytes > fileLength){ //over write and extend
    	int start = position; // we need to start adding sectors from end of the file
	firstSector = divRoundDown(start, SectorSize);
	lastSector = divRoundDown(position + numBytes - 1, SectorSize);
	numSectors = 1 + lastSector - firstSector;
	BitMap* freeMap = getFreeMap();    	
	if (hdr->ExtendFile(freeMap, numSectors) == -1){
		bool noSpace = false;
		ASSERT(noSpace);
	}

	buf = new char[numSectors * SectorSize];

	firstAligned = (position == (firstSector * SectorSize));
	lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

	//TODO LEFTOF DO WE ZERO OUT FOR GAPS?
	
	// read in first and last sector, if they are to be partially modified
	if (!firstAligned)
	    ReadAt(buf, SectorSize, firstSector * SectorSize);	
	if (!lastAligned && ((firstSector != lastSector) || firstAligned))
	    ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

	// copy in the bytes we want to change 
	bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

	// write modified sectors back
	for (i = firstSector; i <= lastSector; i++)	
	    synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
	delete [] buf;
	return numBytes;
    
    }
    else{
	DEBUG('f', "UNKNOWN position condition reached\n");
	bool howDidTheFileWriteGetHere= false;
	ASSERT(howDidTheFileWriteGetHere);
      
    }
      
    
    
    
#else

    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
	return 0;				// check request
    if ((position + numBytes) > fileLength)
	numBytes = fileLength - position;
    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);


//LEFTOFF finding where  we can write in the sector block 
//if not at start or crosses multiple sectors
    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// copy in the bytes we want to change 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    delete [] buf;
    return numBytes;
#endif
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}
