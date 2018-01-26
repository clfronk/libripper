/**
  libripper

  Compile Command: gcc -lcdio -lcdio_cdda -lcdio_paranoia -lcddb -o test ripper.c test.c

**/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#include <cddb/cddb.h>
#include "ripper.h"


/**
	RIPPER_CD_Data * ripperInit()

	Determines the type of cd inserted if a cd is
	inserted and the number of audio and data
	tracks on the cd.  Determines the frame offsets for 
	all tracks. There is no dvd support if a dvd
	is inserted the type will be set to NO_CD

	Returns NULL on error.

*/
ripper_cd_data_t * ripperInit()
{
	ripper_cd_data_t * ripper = malloc(sizeof(ripper_cd_data_t));
	if(ripper == NULL) {
		printf("Error: Unable to allocate more memory.\n");
		return ripper;
	}
	//intialize the structure
	ripper->type = NO_CD;
	ripper->frame_offsets = NULL;
	ripper->drive = NULL;
	ripper->p_paranoia = NULL;
	ripper->numAudioTracks = 0;
	ripper->numDataTracks = 0;
	ripper->totalTracks = 0;
	ripper->format = UNCOMPRESSED_WAV;
	
	ripper->cdio_p = cdio_open(NULL,DRIVER_DEVICE);

	track_t i_tracks;
	track_t first_track_num;

	unsigned int numAudio = 0;
	unsigned int numData = 0;
	unsigned int i,j;

	//check if a driver can be found for the cdrom
	//if not return NO_CD since we can't do anything
	//else
	if(ripper->cdio_p == NULL) {
		printf("Problem finding a driver.\n");
		ripperCDDataDestroy(ripper);
		return NULL;
	}
	
	
	ripper->drive = cdio_cddap_find_a_cdrom(1,NULL);
	
	if(ripper->drive == NULL) {
		printf("An error occured initalizing the drive for ripping.\n");
		return NULL;
	}
	
	cdio_cddap_open(ripper->drive);
	
	cdio_cddap_verbose_set(ripper->drive, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);
	
	ripper->p_paranoia = cdio_paranoia_init(ripper->drive);
	
	first_track_num = cdio_get_first_track_num(ripper->cdio_p);
	//make sure there is a cd inserted
	if(first_track_num == CDIO_INVALID_TRACK) {
		ripper->type = NO_CD;
		ripper = ripperCDDataDestroy(ripper);
	}
	else {
		//get the total number of tracks on the cd data and audio
		i_tracks = cdio_get_num_tracks(ripper->cdio_p);
		ripper->totalTracks = i_tracks;
		//allocate enough space to store all of the frame offsets
		ripper->frame_offsets = calloc(sizeof(int),i_tracks);
		if(ripper->frame_offsets == NULL)
		{
			printf("Error: Allocating memory for frame offsets\n");
			ripper = ripperCDDataDestroy(ripper);
			return ripper;
		}
		//look for audio and data tracks
		for(i = first_track_num,j=1;i <= i_tracks;i++,j++) {
			if(TRACK_FORMAT_AUDIO == cdio_get_track_format(ripper->cdio_p,i)) {
				ripper->frame_offsets[j - 1] = ripperGetFrameOffset(ripper->cdio_p,i,j);
				//return NULL if unable to calculate a frame offset
				if(ripper->frame_offsets[j - 1] == -1) {
					//free allocated memory
					ripperCDDataDestroy(ripper);
					return NULL;
				}
				numAudio++;
			} else {
				ripper->frame_offsets[j-1] =ripperGetFrameOffset(ripper->cdio_p,i,j);
				if(ripper->frame_offsets[j - 1] == -1) {
					ripperCDDataDestroy(ripper);
					return NULL;
				}
				numData++;
			}
		}
		//get the length of the cd n seconds
		ripper->cd_length = ripperGetDiskLength(ripper->cdio_p);
		//make sure we are able to get the disc length
		if(ripper->cd_length == -1) {
			ripperCDDataDestroy(ripper);
			return NULL;
		}
		//determine the cd types
		//this isn't complete but enough for the purposes
		//of this library
		if(numAudio > 0 && numData == 0)
			ripper->type = AUDIO_CD;
		else if(numData > 0 && numAudio == 0)
			ripper->type = DATA_CD;
		else if(numData > 0 && numAudio > 0)
			ripper->type = MIXED_MODE_CD;
		else
			ripper->type = NO_CD; //not sure why this might happen?
	}
	
	ripper->numAudioTracks = numAudio;
	ripper->numDataTracks = numData;
	ripper->totalTracks = numData + numAudio;
	
	return ripper;
}

//ripper_cd_data_t set methods

void setRipperFormat(ripper_cd_data_t * ripper, RIPPER_FORMAT_TYPE fileType)
{
	if(ripper != NULL)
		ripper->format = fileType;
}

//ripper_cd_data_t get methods
RIPPER_CD_TYPE getRipperCDType(ripper_cd_data_t * ripper)
{
	if(ripper != NULL)
		return ripper->type;
	else
		return NO_CD;
}
RIPPER_FORMAT_TYPE getRipperFormat(ripper_cd_data_t * ripper) 
{
	if(ripper != NULL)
		return ripper->format;
	else
		return -1;
}
int getRipperNumAudioTracks(ripper_cd_data_t * ripper) 
{
	if(ripper != NULL)
		return ripper->numAudioTracks;
	else
		return -1;
}
int getRipperNumDataTracks(ripper_cd_data_t * ripper)
{
	if(ripper != NULL)
		return ripper->numDataTracks;
	else
		return -1;
}
int getRipperNumTracks(ripper_cd_data_t * ripper)
{
	if(ripper != NULL)
		return ripper->totalTracks;
	else
		return  -1;
}
int getRipperCDLength(ripper_cd_data_t * ripper)
{
	if(ripper != NULL)
		return ripper->cd_length;
	else
		return -1;
}
		

/**
	ripper_cd_data_t * ripperCDDataDestroy(ripper_cd_data_t *)

	Frees the memory allocated by ripperInit()

	always returns NULL
*/
ripper_cd_data_t * ripperCDDataDestroy(ripper_cd_data_t * ripper)
{
	if(ripper != NULL) {
		//free cdio memory
		cdio_paranoia_free(ripper->p_paranoia);
		cdio_cddap_close(ripper->drive);
		cdio_destroy(ripper->cdio_p);
		free(ripper->frame_offsets);
		free(ripper);
	}	
	return NULL;
}

//determines the frame offset for the specified track on the cd
//used for cddb lookups
//note: track nums start at 1 not 0
//returns the frame offset or -1 on error
int ripperGetFrameOffset(CdIo_t * p_cdio, track_t track,unsigned int track_num)
{
	int offset;
	
	if(track_num == 0) {
		printf("Error: Invalid track number specified.\n");
		offset = -1;
	}
	
	lba_t lba;
	if(p_cdio != NULL) {
		lba = cdio_get_track_lba(p_cdio,track);
		if(lba != CDIO_INVALID_LBA) {
			offset = lba;
		} else {
			printf("Error: Track %d has an invalid lba.\n",track_num);
			offset = -1;
		}
	}
	
	return offset;
}
//Determines the length of the cd in seconds
//returns the length of the cd in seconds or -1 on error
int ripperGetDiskLength(CdIo_t * p_cdio)
{
	int length = 0;
	lba_t lba = cdio_get_track_lba(p_cdio,CDIO_CDROM_LEADOUT_TRACK);
	
	if(lba == CDIO_INVALID_LBA) {
		printf("Error: The leadout track has an invalid lba.\n");
		length = -1;
	}
	
	length = FRAMES_TO_SECONDS(lba);
	
	return length;
}

//initializes a new ripper_cddb_data_t object using an already
//initialized ripper_cd_data_t object.
//returns the ripper_cddb_data_t object or NULL on error
ripper_cddb_data_t * ripperCDDBInit(ripper_cd_data_t * rp)
{
	if(rp != NULL) {
		
		if(rp->type != AUDIO_CD && rp->type != MIXED_MODE_CD) {
			printf("Error: No Audio CD available.\n");
			return NULL;
		}
		
		ripper_cddb_data_t * rp_cddb = malloc(sizeof(ripper_cddb_data_t));
		if(rp_cddb == NULL) {
			printf("Error: Unable to allocated memory for cddb data.\n");
			return NULL;
		}
		
		rp_cddb->totalTracks = 0;
		rp_cddb->disc = cddb_disc_new();
		
		if(rp_cddb->disc == NULL) {
			printf("Error: unable to allocate memory for the disc.\n");
			rp_cddb = ripperCDDBDestroy(rp_cddb);
		} else {
			//set the disc length from the ripper_cd_data_t object
			cddb_disc_set_length(rp_cddb->disc,rp->cd_length);
			
			cddb_track_t * track;
			unsigned int i = 0;
			//add all tracks to the disk
			for(i = 0;i < rp->totalTracks;i++) {
				track = cddb_track_new();
				
				if(track != NULL) {
					cddb_disc_add_track(rp_cddb->disc,track);
					cddb_track_set_frame_offset(track, rp->frame_offsets[i]);
					rp_cddb->totalTracks++;
				} else {
					printf("Error: Unable to allocate memory for track.\n");
					rp_cddb = ripperCDDBDestroy(rp_cddb);
					//may need additional clean up
					break;
				}
			}
		}
		
		//initialize the cddb connection
		if(rp_cddb != NULL) {
			//calculate the disc id, necessary to do queries
			rp_cddb->disc,cddb_disc_calc_discid(rp_cddb->disc);
			//unsigned int id = cddb_disc_get_discid(rp_cddb->disc);
			rp_cddb->conn = cddb_new();
			if(rp_cddb->conn == NULL) {
				printf("Error: Unable to allocate memory for cddb connection.\n");
				rp_cddb = ripperCDDBDestroy(rp_cddb);
			}
		}
		
		
		
		return rp_cddb;
	}
	else
		return NULL;
}
//frees all memory used by the ripper_cddb_data_t object
//always returns NULL
ripper_cddb_data_t * ripperCDDBDestroy(ripper_cddb_data_t * rp_cddb) 
{
	if(rp_cddb != NULL) {
		if(rp_cddb->disc != NULL) {
			//free space used by the disc
			//automatically frees space used by tracks
			cddb_disc_destroy(rp_cddb->disc);
		}
		if(rp_cddb->conn != NULL) {
			cddb_destroy(rp_cddb->conn);
		}
		//free any global resources used by libcddb
		libcddb_shutdown();
		//free the ripper_cddb_data_t object
		free(rp_cddb);
	}
	
	return NULL;
}

//returns the number of cddb matches found for the cd
//returns -1 on error
int ripperGetNumCDDBMatches(ripper_cddb_data_t * rp_cddb)
{
	static num;
	
	if(rp_cddb != NULL) {
		num =  cddb_query(rp_cddb->conn,rp_cddb->disc);
	}
	
	return num;
}

ripper_cddb_query_results_t * ripperCDDBQuery(ripper_cd_data_t * rp,int * numMatches)
{
	if(rp != NULL) {
		
		ripper_cddb_data_t * rp_cddb = ripperCDDBInit(rp);
		if(rp_cddb == NULL) {
			*numMatches = -1;
			return NULL;	
		}
		int matches = ripperGetNumCDDBMatches(rp_cddb);
		//check for no results
		//may modify this behavior later
		if(matches == 0) {
			*numMatches = 0;
			ripperCDDBDestroy(rp_cddb);
			return NULL;
		}
		
		ripper_cddb_query_results_t * cddb_results = calloc(sizeof(ripper_cddb_query_results_t),matches);
		
		if(cddb_results != NULL) {
			int i = 0;
			
			do {
				cddb_read(rp_cddb->conn,rp_cddb->disc);
				cddb_results[i].tracks =  calloc(sizeof(ripper_cddb_track_t),cddb_disc_get_track_count(rp_cddb->disc));
				if(cddb_results[i].tracks == NULL) {
					free(cddb_results);
					printf("Error: Unable to allocate memory for tracks.\n");
					break;
				}
				//add the disk data
				cddb_results[i].numTracks = cddb_disc_get_track_count(rp_cddb->disc);
				//get category
				if(cddb_disc_get_category_str(rp_cddb->disc)) {
				cddb_results[i].category = calloc(sizeof(char),strlen(cddb_disc_get_category_str(rp_cddb->disc))+1);
				strcpy(cddb_results[i].category,cddb_disc_get_category_str(rp_cddb->disc));
				}
				//get artist				
				if(cddb_disc_get_artist(rp_cddb->disc)) {
				cddb_results[i].artist = calloc(sizeof(char),strlen(cddb_disc_get_artist(rp_cddb->disc))+1);
				strcpy(cddb_results[i].artist,cddb_disc_get_artist(rp_cddb->disc));
				}
				//get title
				if(cddb_disc_get_title(rp_cddb->disc)) {
				cddb_results[i].title = calloc(sizeof(char),strlen(cddb_disc_get_title(rp_cddb->disc))+1);
				strcpy(cddb_results[i].title,cddb_disc_get_title(rp_cddb->disc));
				}
				//get genre
				if(cddb_disc_get_genre(rp_cddb->disc)) {
				cddb_results[i].genre = calloc(sizeof(char),strlen(cddb_disc_get_genre(rp_cddb->disc))+1);
				strcpy(cddb_results[i].genre,cddb_disc_get_genre(rp_cddb->disc));
				}
				//get year
				cddb_results[i].year = cddb_disc_get_year(rp_cddb->disc);
				//get extended data
				if(cddb_disc_get_ext_data(rp_cddb->disc)) {
				cddb_results[i].ext_data = calloc(sizeof(char),strlen(cddb_disc_get_ext_data(rp_cddb->disc))+1);
				strcpy(cddb_results[i].ext_data,cddb_disc_get_ext_data(rp_cddb->disc));
				}
				
				
				//add the track data
				int j = 0;
				cddb_track_t * curTrack = cddb_disc_get_track_first(rp_cddb->disc);
				while(curTrack != NULL) {
					//get track length
					cddb_results[i].tracks[j].length = cddb_track_get_length(curTrack);
					//get track title
					if(cddb_track_get_title(curTrack)) {
					cddb_results[i].tracks[j].title = calloc(sizeof(char),strlen(cddb_track_get_title(curTrack))+1);
					strcpy(cddb_results[i].tracks[j].title,cddb_track_get_title(curTrack));
					}
					if(cddb_track_get_artist(curTrack)) {
						cddb_results[i].tracks[j].artist = calloc(sizeof(char),strlen(cddb_track_get_artist(curTrack))+1);
						strcpy(cddb_results[i].tracks[j].artist,cddb_track_get_artist(curTrack));
					}
					curTrack = cddb_disc_get_track_next(rp_cddb->disc);
					j++;
				}
				
				i++;
				
			} while(cddb_query_next(rp_cddb->conn,rp_cddb->disc));
			
		}
		*numMatches = matches;
		ripperCDDBDestroy(rp_cddb);
		return cddb_results;
	} else {
		*numMatches = -1;
		return NULL;
	}
}

//frees all memory used by ripper_cddb_query_results_t
//always returns NULL;
ripper_cddb_query_results_t * ripperCDDBQueryDestroy(ripper_cddb_query_results_t * cddb_res)
{
	if(cddb_res != NULL) {
		int length = ripperGetNumCDDBMatches(NULL);
		
		int i;
		//free each result
		for(i = 0;i<length;i++) {
			int j;
			//free the data from each track on
			//the current result
			if(cddb_res[i].tracks != NULL) {
				for(j = 0;j < cddb_res[i].numTracks;j++) {
					free(cddb_res[i].tracks[j].title);	
					free(cddb_res[i].tracks[j].artist);
				}
			}
			//free all other disc data
			free(cddb_res[i].tracks);
			free(cddb_res[i].category);
			free(cddb_res[i].artist);
			free(cddb_res[i].title);
			free(cddb_res[i].genre);
			free(cddb_res[i].ext_data);
			
		}
			
		free(cddb_res);
	}
	
	return NULL;
}

//cddb accessor methods
char * getRipperCDDBCategory(const ripper_cddb_query_results_t * rp_cddb)
{
	if(rp_cddb != NULL) {
		return rp_cddb->category;	
	} else
		return NULL;
}

char * getRipperCDDBArtist(const ripper_cddb_query_results_t * rp_cddb) 
{
	if(rp_cddb != NULL) {
		return rp_cddb->artist;
	} else
		return NULL;
}

char * getRipperCDDBTitle(const ripper_cddb_query_results_t * rp_cddb)
{
	if(rp_cddb != NULL) {
		return rp_cddb->title;
	} else
		return NULL;
}

char * getRipperCDDBGenre(const ripper_cddb_query_results_t * rp_cddb)
{
	if(rp_cddb != NULL) {
		return rp_cddb->genre;
	} else
		return NULL;
}

char * getRipperCDDBExtData(const ripper_cddb_query_results_t * rp_cddb)
{
	if(rp_cddb != NULL) {
		return rp_cddb->ext_data;
	} else
		return NULL;
}

int getRipperCDDBYear(const ripper_cddb_query_results_t * rp_cddb)
{
	if(rp_cddb != NULL) {
		return rp_cddb->year;
	} else
		return -1;
}

char * getRipperCDDBTrackTitle(const ripper_cddb_query_results_t * rp_cddb,unsigned int trackNum)
{
	if(rp_cddb != NULL && trackNum <= rp_cddb->numTracks) {
		return rp_cddb->tracks[trackNum - 1].title;
	} else
		return NULL;
}

char * getRipperCDDBTrackArtist(const ripper_cddb_query_results_t * rp_cddb,unsigned int trackNum)
{
	if(rp_cddb != NULL && trackNum <= rp_cddb->numTracks) {
		return rp_cddb->tracks[trackNum - 1].artist;
	} else
		return NULL;
}

//returns the length of the track in seconds
int getRipperCDDBTrackLength(const ripper_cddb_query_results_t * rp_cddb,unsigned int trackNum)
{
	if(rp_cddb != NULL && trackNum <= rp_cddb->numTracks) {
		return rp_cddb->tracks[trackNum - 1].length;
	} else
		return -1;
}

void setRipperCDDBCategory(ripper_cddb_query_results_t * rp_cddb,char * category)
{
	if(rp_cddb != NULL && category != NULL) {
		free(rp_cddb->category);
		rp_cddb->category = calloc(sizeof(char),strlen(category)+1);
		strcpy(rp_cddb->category,category);
	}
}

void setRipperCDDBArtist(ripper_cddb_query_results_t * rp_cddb, char * artist)
{
	if(rp_cddb != NULL && artist != NULL) {
		free(rp_cddb->artist);
		rp_cddb->artist = calloc(sizeof(char),strlen(artist)+1);
		strcpy(rp_cddb->artist,artist);
	}
}

void setRipperCDDBTitle(ripper_cddb_query_results_t * rp_cddb, char * title)
{
	if(rp_cddb != NULL && title != NULL) {
		free(rp_cddb->title);
		rp_cddb->title = calloc(sizeof(char),strlen(title)+1);
		strcpy(rp_cddb->title,title);
	}
}

void setRipperCDDBGenre(ripper_cddb_query_results_t * rp_cddb,char * genre)
{
	if(rp_cddb != NULL && genre != NULL) {
		free(rp_cddb->genre);
		rp_cddb->genre = calloc(sizeof(char),strlen(genre)+1);
		strcpy(rp_cddb->genre,genre);
	}
}

void setRipperCDDBExtData(ripper_cddb_query_results_t * rp_cddb,char * ext_data)
{
	if(rp_cddb != NULL && ext_data != NULL) {
		free(rp_cddb->ext_data);
		rp_cddb->ext_data = calloc(sizeof(char),strlen(ext_data)+1);
		strcpy(rp_cddb->ext_data,ext_data);
	}
}

void setRipperCDDBYear(ripper_cddb_query_results_t * rp_cddb, int year)
{
	if(rp_cddb != NULL) {
		rp_cddb->year = year;
	}
}

void setRipperCDDBTrackTitle(ripper_cddb_query_results_t * rp_cddb,char * title,unsigned int trackNum)
{
	if(rp_cddb != NULL && title != NULL,trackNum <= rp_cddb->numTracks) {
		free(rp_cddb->tracks[trackNum - 1].title);
		rp_cddb->tracks[trackNum - 1].title = calloc(sizeof(char),strlen(title));
		strcpy(rp_cddb->tracks[trackNum-1].title,title);
	}
}

void setRipperCDDBTrackArtist(ripper_cddb_query_results_t * rp_cddb,char * artist,unsigned int trackNum)
{
	if(rp_cddb != NULL && artist != NULL && trackNum <= rp_cddb->numTracks) {
		free(rp_cddb->tracks[trackNum - 1].artist);
		rp_cddb->tracks[trackNum - 1].artist = calloc(sizeof(char),strlen(artist));
		strcpy(rp_cddb->tracks[trackNum - 1].artist,artist);
	}
}

void setRipperCDDBTrackLength(ripper_cddb_query_results_t * rp_cddb,int length,unsigned int trackNum)
{
	if(rp_cddb != NULL && trackNum <= rp_cddb->numTracks) {
		rp_cddb->tracks[trackNum - 1].length = length;
	}
}

//writes the header information for the wav file
//to the inputed file.  The file pointer must be open
//for writing and be pointing at the beginning of the
//file for the correct result 
//returns 1 on success and returns -1 on error
int ripperWriteWavHeader(FILE * fp, int data_size) 
{
	if(fp == NULL) {
		return -1;
	}
	
	int file_size = data_size + WAV_HEADER_SIZE;
	
	//write the riff type chunk
	fwrite(WAV_HDR_CHNK_ID,sizeof(char),4,fp);
	//write file size
	fwrite(&file_size,sizeof(int),1,fp);
	//write riff types
	fwrite(&RIFF_TYPE,sizeof(char),4,fp);
	
	//write the format chunk
	
	//write the format id
	fwrite(FORMAT_ID,sizeof(char),4,fp);
	//write the chunk size
	int chunk_size = FMT_CHUNK_SIZE + (EXTRA_FORMAT_BYTES / 8);
	fwrite(&chunk_size,sizeof(int),1,fp);
	//write compession code - currently no compression
	fwrite(&COMPRESSION_CODE,sizeof(short),1,fp);
	//write the number of channels currently 2 (stereo)
	fwrite(&NUM_CHANNELS,sizeof(short),1,fp);
	//write the sample rate
	fwrite(&SAMPLE_RATE,sizeof(int),1,fp);
	//write the avg bytes per second
	fwrite(&AVG_BYTES_PER_SEC,sizeof(int),1,fp);
	//write the block align
	fwrite(&BLOCK_ALIGN,sizeof(short),1,fp);
	//write bits per sample
	fwrite(&BITS_PER_SAMPLE,sizeof(short),1,fp);
	
	//write the data chunk
	
	//write the data id
	fwrite(DATA_ID,sizeof(char),4,fp);
	//write the data sizeof
	fwrite(&data_size,sizeof(int),1,fp);	
	
	return 1;
}

int ripperRipTrack(ripper_cd_data_t * ripper,int trackNum, char * filename)
{
	if(filename == NULL) {
		printf("Error: No filename was specified.\n");
		return -1;
	}
	
	//make sure that the track is an audio track
	if(!cdio_cddap_track_audiop(ripper->drive,trackNum)) {
		printf("Error: Track %d is not an audio track.\n",trackNum);
		return -1;
	}
	
	//attempt to get the first and last sectors of the track
	lsn_t f_sector = cdio_cddap_track_firstsector(ripper->drive,trackNum);
	lsn_t l_sector = cdio_cddap_track_lastsector(ripper->drive,trackNum);
	
	//make sure we are able to get the first and last sectors
	//of the track to be ripped.
	if(f_sector == -1 || l_sector == -1) {
		printf("Error: Unable to get track information.\n");
		return -1;
	}
	
	int data_size = CDIO_CD_FRAMESIZE_RAW * (l_sector - f_sector + 1);
	
	FILE * fp = fopen(filename,"w");
	ripperWriteWavHeader(fp,data_size);
	
	if(fp == NULL) {
		printf("Error: Unable to open file %s for writing.\n",filename);
		return -1;
	}
	
	cdio_paranoia_seek(ripper->p_paranoia,f_sector,SEEK_SET);
	
	lsn_t i;
	
	// 	read in the track
	for(i = f_sector;i <= l_sector; i++) {
		int16_t * p_buffer = cdio_paranoia_read(ripper->p_paranoia,NULL);
		char * err_msg = cdio_cddap_errors(ripper->drive);
		char * inf_msg = cdio_cddap_messages(ripper->drive);
		
		if(err_msg) {
			free(err_msg);
		}
		if(inf_msg) {
			free(inf_msg);
		}
		
		if(!p_buffer) {
			printf("A read error occured. Aborting..\n");
			fclose(fp);
			return -1;	
		} else {
			fwrite(p_buffer,CDIO_CD_FRAMESIZE_RAW,1,fp);
		}
	}
	
	fclose(fp);
	//move the starting offset back to the start of the cd
	cdio_paranoia_seek(ripper->p_paranoia,-l_sector,SEEK_SET);

	return 1;
}
