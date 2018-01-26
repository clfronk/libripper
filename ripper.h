#ifndef RIPPER_H_
#define RIPPER_H_
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#include <cddb/cddb.h>

//wav header constants
const static char WAV_HDR_CHNK_ID[] = "RIFF";
const static char RIFF_TYPE[] = "WAVE";
const static char FORMAT_ID[] = "fmt ";
const static char DATA_ID[] = "data";
//may make some of the following definitions configurable later.
const static int FMT_CHUNK_SIZE = 16;
const static int SAMPLE_RATE = 44100;
const static short COMPRESSION_CODE = 1;
const static short NUM_CHANNELS = 2;
const static short BITS_PER_SAMPLE = 16;
const static short EXTRA_FORMAT_BYTES = 0;
//block align = bits_per_sample / 8 * num_channels
const static short BLOCK_ALIGN = 4;
//avg_bytes_per_second = sample_rate * block_align
const static int AVG_BYTES_PER_SEC = 176400;
//size of wav header used in determing wav file size
const static int WAV_HEADER_SIZE = 36;

typedef
	enum RIPPER_CD_TYPE { AUDIO_CD, DATA_CD, MIXED_MODE_CD, NO_CD }
RIPPER_CD_TYPE;

typedef 
	enum RIPPER_FORMAT_TYPE { RAW_CD_DATA, UNCOMPRESSED_WAV }
RIPPER_FORMAT_TYPE;

typedef struct ripper_cd_data_t {
	RIPPER_CD_TYPE type;
	RIPPER_FORMAT_TYPE format;
	CdIo_t * cdio_p;
	cdrom_drive_t * drive;
	cdrom_paranoia_t * p_paranoia;
	unsigned int numAudioTracks;
	unsigned int numDataTracks;
	unsigned int totalTracks;
	int * frame_offsets;
	unsigned int cd_length;
}ripper_cd_data_t;

typedef struct ripper_cddb_data_t {
	cddb_disc_t * disc;
	cddb_conn_t * conn;
	unsigned int totalTracks;
}ripper_cddb_data_t;

//stores information for each track
//on the cd information.
//track length is in seconds
typedef struct ripper_cddb_track_t {
	char * title;
	char * artist;
	int length;
}ripper_cddb_track_t;
//stores all of the information
//retrieved from the cddb query
//tracks is an array of ripper_cddb_track_t structs
//of length numTracks
typedef struct ripper_cddb_query_results_t {
	char * category;
	char * artist;
	char * title;
	char * genre;
	char * ext_data;
	unsigned int year;
	int numTracks;
	ripper_cddb_track_t * tracks;
}ripper_cddb_query_results_t;

//determines the type of cd and the number of audio
//and data tracks on the cd.
ripper_cd_data_t * ripperInit();

//frees the memory allocated in ripperInit()
//always returns NULL
ripper_cd_data_t * ripperCDDataDestroy(ripper_cd_data_t *);

//rips the inputed track number and writes
//it to the inputed file.
//
//returns 1 for sucess and -1 on error
int ripperRipTrack(ripper_cd_data_t *,int,char *);

//writes the wav header to the inputed
//file.  returns 1 on sucess and -1 on error
int ripperWriteWavHeader(FILE * fp,int data_size);

//determine the frame offset for the inputed track
//this is needed in order to calculate the discid used
//for cddb queries.
//returns the frame offset or -1 on error
int ripperGetFrameOffset(CdIo_t *,track_t,unsigned int);

//calculates the length of the cd in seconds
//returns the length of the cd or -1 on error
int ripperGetDiskLength(CdIo_t *);

//ripper set methods
void setRipperFormat(ripper_cd_data_t * ripper, RIPPER_FORMAT_TYPE fileType);

//ripper get methods 
//return -1 if a null pointer is passed
//except ripperGetCDType which returns NO_CD 
//when a null pointer has passed
RIPPER_FORMAT_TYPE getRipperFormat(ripper_cd_data_t * ripper);
RIPPER_CD_TYPE getRipperCDType(ripper_cd_data_t * ripper);
int getRipperNumAudioTracks(ripper_cd_data_t * ripper);
int getRipperNumDataTracks(ripper_cd_data_t * ripper);
int getRipperNumTracks(ripper_cd_data_t * ripper);
//returns the length of the cd in seconds
//this may not be correct if there are data
//tracks?
int getRipperCDLength(ripper_cd_data_t * ripper);

//initializes a new ripper_cddb_data_t 
//Creates all necessary objects through libcddb
//sets the disc length, adds all tracks to the disc
//gets the discid, creates the connection object
//returns a newly allocated ripper_cddb_data_t object(well not really an object)
//or NULL on error
ripper_cddb_data_t * ripperCDDBInit(ripper_cd_data_t *);

//frees all memory used by the ripper_cddb_data_t object
//always returns NULL
ripper_cddb_data_t * ripperCDDBDestroy(ripper_cddb_data_t *);

//returns the number of cddb matches found for the
//disc or -1 on error
int ripperGetNumCDDBMatches(ripper_cddb_data_t *);

//retrieves all available cddb query results
//returns an array of ripper_cddb_query_results_t *
//numMatches will be set to the number of results found
//numMatches is set to -1 on error
//returns NULL on error or if no results are found
ripper_cddb_query_results_t * ripperCDDBQuery(ripper_cd_data_t *,int * numMatches);

//frees all space used by ripper_cddb_query_results_t
//always returns NULL
ripper_cddb_query_results_t * ripperCDDBQueryDestroy(ripper_cddb_query_results_t *);

//cddb accessor methods
char * getRipperCDDBCategory(const ripper_cddb_query_results_t *);
char * getRipperCDDBArtist(const ripper_cddb_query_results_t *);
char * getRipperCDDBTitle(const ripper_cddb_query_results_t *);
char * getRipperCDDBGenre(const ripper_cddb_query_results_t *);
char * getRipperCDDBExtData(const ripper_cddb_query_results_t *);
//returns -1 if a null pointer is passed
int getRipperCDDBYear(const ripper_cddb_query_results_t *);
//cddb track accessor methods
//track number parameters start at 1
char * getRipperCDDBTrackTitle(const ripper_cddb_query_results_t *,unsigned int);
char * getRipperCDDBTrackArtist(const ripper_cddb_query_results_t *,unsigned int);
//returns the length of the track in seconds
//returns -1 if a null pointer or invalid track # is passed
int getRipperCDDBTrackLength(const ripper_cddb_query_results_t *,unsigned int);

//cddb mutator methods
void setRipperCDDBCategory(ripper_cddb_query_results_t *,char * category);
void setRipperCDDBArtist(ripper_cddb_query_results_t *,char *  artist);
void setRipperCDDBTitle(ripper_cddb_query_results_t *,char * title);
void setRipperCDDBGenre(ripper_cddb_query_results_t *,char * genre);
void setRipperCDDBExtData(ripper_cddb_query_results_t *,char * ext_data);
void setRipperCDDBYear(ripper_cddb_query_results_t *,int year);
//cddb track mutator methods
//all char * passed will be freed when ripperCDDBDestroy is called so
//they must be able to be freed using free
void setRipperCDDBTrackTitle(ripper_cddb_query_results_t *,char * title,unsigned int trackNum);
void setRipperCDDBTrackArtist(ripper_cddb_query_results_t *,char * artist, unsigned int trackNum);
void setRipperCDDBTrackLength(ripper_cddb_query_results_t *, int length,unsigned int trackNum);

#endif
