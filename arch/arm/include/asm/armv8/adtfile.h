/*
 *
 * Copyright(C) 2011-2013 Marvell International Ltd.
 * All Rights Reserved
 *
 * A: Architecture
 * D: Definition
 * T: Trace
 *
 * Basic record file format:
 * 1. the header is stored as little endian
 * 2. Each record is aligned to a 4 byte boundary
 * 4. bit 23.. 0: record length in 32bit values (excluding the header) in 4 bytes.
 * 5. bit 31..24: record typ
 * 6. Strings are 0-byte terminated and the output is always aligned to write 4 byte length
 * 7. Only one ADTF file can be open in any given time
 * 8. Except for ADTF_Open, 0 is always a success return
 * 9. all Id are 1-based, 0 means not applicable or avaialg,
 * 10. ID must be unqiue without each type.
 *
 * The 1st record must be a StartRecord and the file is finished with a end record
 * Both records must not occure anywhere in the file. All other records must be
 * Encapuslated between these 2 records. Unknown records are shiped by the
 * processing tool.
 * The max record length is 64 MByte. If a record can exceed this length it must
 * be split into multiple records (see Trace Buffer Example)
 *
 * void * ADTF_Open(const char *pName, ADTF_FILETYPE Flag);
 *   Open a ADTF file to read (ADTF_READ) or write (ADTF_WRITE) on success, the function
 *   return a valid pointer (not NULL)
 *   Note: creates the start record
 *
 * int    ADTF_Close(void *fp);
 *   close the file and free all associated data structures
 *   Note: creates the EndRecord
 *
 * int    ADTF_Tool(void *pF, const char *pName, const char *pVersion, const char *pTime);
 *   Create a Tool record to identify the tool, version and time of this file
 *
 * int    ADTF_Architecture(void *pF, int cores, int traceSources, int traceSinks, const char *pName);
 *   Specify the architecture definition
 *   cores         : number of trace sources
 *   trace sources : number of trace sources
 *   trace sinks   : number of the TraceSink
 *   the architecture name: name of the SoC
 *
 * int    ADTF_Core(void *pF, int coreId, int smpId, const char *pName, int count, unsigned int *pRegSet);
 *   Define a core, with a uniq core Id (starting with 1), the SMP group, architecture name, and the core register set
 *
 * int    ADTF_TraceSource(void *pF, int sourceId, int coreId, int sinkId, const char *pName, int count,
 *                         unsigned int *pRegSet);
 *   Define a trace source, core association, sink associateion, name, and register dump
 *   SourceID must be unique
 *   coreId==0: no associated with a particualr core, otherwise the core Id must occur
 *
 * int    ADTF_TraceSink(void *pF, int sinkId, const char *pName, int count, unsigned int *pRegSet);
 *   Specify a trace sink, the strace skinId must be uniq, this record also contains the sink
 *   register set. This is a final sink witohut any output
 *
 * int    ADTF_TraceSinkInOut(void *pF, int sinkId, const char *pName, int sinkIdOut, int count, unsigned int *pRegSet);
 *   Same as Trace Sink but with a FIFO output, the output ID is specifed in the sourceId
 *   this sink can pass data through
 *
 * int    ADTF_TraceReplicator(void *pF, int sinkId, const char *pName, int sinkId1, int sinkId2);
 *   make 2 sink IDs out of 1
 *
 * int    ADTF_TraceFunnel(void *pF, int sourceId, const char *pName, int count, unsigned int *pSourceId);
 *   Specifes the merges multiple trace source streams into one
 *
 * int    ADTF_TraceBuffer(void *pF, int sinkId, unsigned int offset, int count, void *pBuffer);
 * int    ADTF_TraceBuffer64(void *pF, int sinkId, unsigned long long offset, int count, void *pBuffer);
 *   Add a trace buffer conent for a specific sinkId, the sinkId must be specified
 *
 * Helper function to read theough the file.
 * int    ADTF_SeekNextRecord(void *pF, int ID, int flag, int optID1, int optID2, int optID3);
 * int    ADTF_GetRecordLength(void *pF);
 * int    ADTF_SetFirstRecord(void *pF);
 * int    ADTF_GetRecordType(void *pF, int *pType);
 * int    ADTF_GetRecordData(void *pF, void *pBuffer);
 * int    ADTF_AddRecord(void *pF, void *pRecord, unsigned int length);
 * int    ADTF_GetNInt(void *pF, int count, int *pInt);
 * int    ADTF_GetInt(void *pF, int *pInt);
 * int    ADTF_GetString(void *pF, char *pInt);
 *
 * Typical record layout
 *  ADTF_eStartRecord
 *  ADTF_eToolDescription
 *  ADTF_eArchitectureDescription
 *  ADTF_eCoreDescription[n]
 *  ADTF_eTraceSource[m]
 *  ADTF_eTraceSink|ADTF_eTraceSinkInOut|ADTF_eTraceFunnel|ADTF_TraceReplicator[k]
 *  ADTF_eTraceBuffer[]
 *  ADTF_eTraceBuffer64[]
 *  ADTF_eEndRecord
 *
 * Example:
 *  =======================================================
 * Simple Single Core Example:
 * {n}: Core number
 * (n): sourceID
 * [n]: SinkId
 * CORE   PJ4B{1}
 *         |
 * TRACE  ETM(1)
 *         |
 *         |[1]
 *         |
 * BUFFER ETB
 *
 * START
 *    ADTF_eStartRecord
 *
 * TOOL:
 *     ADTF_eToolDescription("XDB", "V5.7.10", "2014/06/12 12:23:34")
 *
 * SOC
 *     ADTF_eArchitectureDescription(1, 1, 1, "PJ4B_single")
 *     ADTF_eCoreDescription(1, 0, "PJ4B", 16, aRegBuf[0])
 *     ADTF_eTraceSource( 1, 1, 1, "ETM",  64, aTraceSource[0])
 *     ADTF_eTraceSink(1, "ETB", 8, aETB[0])
 *
 * TRACEBUFFERS
 *     ADTF_eTraceBuffer(1, 0, 0x1000/4, aETBs[0])
 *
 * END
 *     ADTF_eEndRecord()
 *
 * ========================================================
 * Simple Multi Core Example, individual buffer:
 * {n}: Core number
 * (n): sourceID
 * [n]: SinkId
 * CORE   PJ4B{1} PJ4B{2}
 *         |       |
 * TRACE  ETM(1)  ETM(2)
 *         |       |
 *         |[1]    |[2]
 *         |       |
 * BUFFER ETB     ETB
 *
 *
 * START
 *    ADTF_eStartRecord
 *
 * TOOL:
 *     ADTF_eToolDescription("XDB", "V5.7.10", "2014/06/12 12:23:34")
 *
 * SOC
 *     ADTF_eArchitectureDescription(2, 2, 2, "PJ4B_dual")
 *     ADTF_eCoreDescription(1, 1, "PJ4B", 16, aRegBuf[0])
 *     ADTF_eCoreDescription(2, 1, "PJ4B", 16, aRegBuf[1])
 *     ADTF_eTraceSource( 1, 1, 1, "ETM",  64, aTraceSource[0])
 *     ADTF_eTraceSource( 2, 2, 2, "ETM",  64, aTraceSource[1])
 *     ADTF_eTraceSink(1, "ETB", 8, aETB[0])
 *     ADTF_eTraceSink(2, "ETB", 8, aETB[0])
 *
 * TRACEBUFFERS
 *     ADTF_eTraceBuffer(1, 0, 0x1000/4, aETBs[0])
 *     ADTF_eTraceBuffer(2, 0, 0x1000/4, aETBs[1])
 *
 * END
 *     ADTF_eEndRecord()
 *
 *
 * ========================================================
 * Simple Multi Core Example Global buffer:
 * {n}: Core number
 * (n): sourceID
 * [n]: SinkId
 * CORE   PJ4B{1} PJ4B{2} PJ4B{3}
 *         |       |       |
 * TRACE  ETM(1)  ETM(2)  ETM(3)
 *         |       |       |
 *         |[1]    |[2]    |[3]
 *         |       |       |
 *         +-------+-------+
 *                 |
 * CSTF           CSTF
 *                 |
 *                 |[4]
 *                 |
 * BUF            TMC
 * START
 *    ADTF_eStartRecord
 *
 * TOOL:
 *     ADTF_eToolDescription("XDB", "V5.7.10", "2014/06/12 12:23:34")
 *
 * SOC
 *     ADTF_eArchitectureDescription(3, 3, 2, "PJ4B_tri")
 *     ADTF_eCoreDescription(1, 1, "PJ4B", 16, aRegBuf[0])
 *     ADTF_eCoreDescription(2, 1, "PJ4B", 16, aRegBuf[1])
 *     ADTF_eCoreDescription(3, 1, "PJ4B", 16, aRegBuf[2])
 *     ADTF_eTraceSource( 1, 1, 1, "ETM",  64, aTraceSource[0])
 *     ADTF_eTraceSource( 2, 2, 2, "ETM",  64, aTraceSource[1])
 *     ADTF_eTraceSource( 3, 3, 3, "ETM",  64, aTraceSource[2])
 *     ADTF_eTraceFunnel(4, "CSTF", 2, {1,2,3})
 *     ADTF_eTraceSink(4, "ETB", 8, aETB[0])
 *
 * TRACEBUFFERS
 *     ADTF_eTraceBuffer(4, 0, 0x1000/4, aETBs[0])
 *
 * END
 *     ADTF_eEndRecord()
 *
 * ========================================================
 * Complex multi core example:
 * {n}: Core number
 * (n): sourceID
 * [n]: SinkId
 * CORE  966{1}  CA53{2} CA53{3} CA53{4} CA53{5} ARM946{6} MSA7{7}
 *                |       |       |       |
 * TRACE         ETM(1)  ETM(2)  ETM(3)  ETM(4)                     ITM(5) STM(6)
 *                |       |       |       |                          |      |
 *                |[1]    |[2]    |[3]    |[4]                       |[5]   |[6]
 *                |       |       |       |                          |      |
 * BUF           TMC[1]  TMC[2]  TMC[3]  TMC[4]                      |      |
 *                |      |       |       |                           |      |
 *                |[7]   |[8]    |[9]    |[10]                       |      |
 *                |      |       |       |                           |      |
 *                +------+---+---+-------+                           |      |
 *                           |                                       |      |
 * CSTF                     CSTF                                     |      |
 *                           |                                       |      |
 *                           |[11]                                   |      |
 *                           |                                       |      |
 *                           +---+-----------------------------------+------+
 * CSTF                         CSTF
 *                               |
 *                               |[12]
 *                               |
 * BUF                          TMC
 *                               |
 *                               |[13]
 *                               |
 * TPIU                         TPIU
 *
 * START
 *    ADTF_eStartRecord
 *
 * TOOL:
 *     ADTF_eToolDescription("XDB", "V5.7.10", "2014/06/12 12:23:34")
 *
 * SOC
 *     ADTF_eArchitectureDescription(7, 6, 8, "PXA1928_A0")
 *     ADTF_eCoreDescription(1, 0, "DRAGONITE", 16, aRegBuf[0])
 *     ADTF_eCoreDescription(2, 1, "CA53",      64, aRegBufCA53[0])
 *     ADTF_eCoreDescription(3, 1, "CA53",      64, aRegBufCA53[1])
 *     ADTF_eCoreDescription(4, 1, "CA53",      64, aRegBufCA53[2])
 *     ADTF_eCoreDescription(5, 1, "CA53",      64, aRegBufCA53[3])
 *     ADTF_eCoreDescription(6, 0, "ARM946",    16, aRegBuf[1])
 *     ADTF_eCoreDescription(7, 0, "MSA",       16, aRegBuf[2])
 *
 *     ADTF_eTraceSource( 1, 2, 1, "ETM",  64, aTraceSource[0])
 *     ADTF_eTraceSource( 2, 3, 2, "ETM",  64, aTraceSource[1])
 *     ADTF_eTraceSource( 3, 4, 3, "ETM",  64, aTraceSource[2])
 *     ADTF_eTraceSource( 4, 5, 4, "ETM",  64, aTraceSource[3])
 *     ADTF_eTraceSource( 5, 0, 5, "ITM",   8, aITM[0])
 *     ADTF_eTraceSource( 6, 0, 6, "STM",  16, aSTM[0])
 *
 *     ADTF_eTraceSinkWOutput(1, "TMC",  7,  8, aTMC[0])
 *     ADTF_eTraceSinkWOutput(2, "TMC",  8,  8, aTMC[1])
 *     ADTF_eTraceSinkWOutput(3, "TMC",  9,  8, aTMC[2])
 *     ADTF_eTraceSinkWOutput(4, "TMC", 10,  8, aTMC[3])
 *
 *     ADTF_eTraceFunnel(11, "CSTF", 4, { 7, 8, 9, 10})
 *     ADTF_eTraceFunnel(12, "CSTF", 3, { 5, 6,11})
 *
 *     ADTF_eTraceSinkWOutput(12, "TMC", 13,  8, aTMC[4])
 *
 *     ADTF_eTraceSink(13, "TPIU", 4, aTPIU[0])
 *
 * TRACEBUFFERS
 *     ADTF_eTraceBuffer( 1, 0,  0x120/4, aTMCs[0])
 *     ADTF_eTraceBuffer( 2, 0,  0x110/4, aTMCs[1])
 *     ADTF_eTraceBuffer( 3, 0,  0x150/4, aTMCs[2])
 *     ADTF_eTraceBuffer( 4, 0,   0x10/4, aTMCs[3])
 *     ADTF_eTraceBuffer(12, 0, 0x1000/4, aTMCs[4])
 *     ADTF_eTraceBuffer64(13, 0x000000, 0x100000 / 4, aMJPbuffer)
 *     ADTF_eTraceBuffer64(13, 0x100000, 0x100000 / 4, aMJPbuffer)
 *     ADTF_eTraceBuffer64(13, 0x200000, 0x100000 / 4, aMJPbuffer)
 *     ADTF_eTraceBuffer64(13, 0x300000, 0x100000 / 4, aMJPbuffer)
 *     ADTF_eTraceBuffer64(13, 0x400000,   0x1234 / 4, aMJPbuffer)
 *
 * END
 *     ADTF_eEndRecord()
 *
 */

#ifndef ADTFILE_H
#define ADTFILE_H

typedef enum {
  ADTF_WRITE = 0x4711,
  ADTF_READ  = 0x0815
} ADTF_FILETYPE;

#if __cplusplus
extern "C" {
#endif
typedef enum {
  ADTF_eStartRecord             = 0x00,
  ADTF_eToolDescription         = 0x01,
  ADTF_eArchitectureDescription = 0x02,
  ADTF_eTraceSink               = 0x03,
  ADTF_eTraceSource             = 0x04,
  ADTF_eTraceBuffer             = 0x05,
  ADTF_eCoreDescription         = 0x06,
  ADTF_eTraceBuffer64           = 0x07,
  ADTF_eTraceSinkWOutput        = 0x08,
  ADTF_eTraceFunnel             = 0x09,
  ADTF_eTraceReplicator         = 0x0a,

  ADTF_eToolRangeStart = 0xc0,
  ADTF_eToolRangeEnd   = 0xef,
  ADTF_eEndRecord      = 0xff
} ADTF_RecordTypes;

/* MAX record length in integers */
#define ADTF_MAXLENGTH (16*1024*1024 - 1)

void * ADTF_Open(const char *pName, ADTF_FILETYPE Flag);
int    ADTF_Close(void *fp);

int    ADTF_Tool(void *pF, const char *pName, const char *pVersion, const char *pTime);
int    ADTF_Architecture(void *pF, int cores, int traceSources, int traceSinks, const char *pName);
int    ADTF_TraceSource(void *pF, int sourceId, int coreId, int sinkId, const char *pName, int count,
                        unsigned int *pRegSet);

int    ADTF_TraceSink(void *pF, int sinkId, const char *pName, int count, unsigned int *pRegSet);
int    ADTF_TraceSinkInOut(void *pF, int sinkId, const char *pName, int sinkIdOut, int count, unsigned int *pRegSet);
int    ADTF_TraceReplicator(void *pF, int sinkId, const char *pName, int sinkId1, int sinkId2);
int    ADTF_TraceFunnel(void *pF, int sinkId, const char *pName, int count, unsigned int *pRegSet);

int    ADTF_TraceBuffer(void *pF, int sinkId, unsigned int offset, int count, void *pBuffer);
int    ADTF_TraceBuffer64(void *pF, int sinkId, unsigned long long offset, int count, void *pBuffer);

int    ADTF_Core(void *pF, int id, int smpid, const char *pName, int count, unsigned int *pRegSet);
int    ADTF_SeekNextRecord(void *pF, int ID, int flag, int optID1, int optID2, int optID3);
int    ADTF_GetRecordLength(void *pF);
int    ADTF_SetFirstRecord(void *pF);
int    ADTF_GetRecordType(void *pF, int *pType);
int    ADTF_GetRecordData(void *pF, void *pBuffer);/* buffer must be large enought to hold the record data */
int    ADTF_AddRecord(void *pF, void *pRecord, unsigned int length);

int    ADTF_GetNInt(void *pF, int count, int *pInt);
int    ADTF_GetInt(void *pF, int *pInt);
int    ADTF_GetString(void *pF, char *pInt);

extern void adtf_dump_all_trace_buffer(void);

#if __cplusplus
};
#endif

#endif /* ADTFILE_H */
