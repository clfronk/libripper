#include <stdio.h>
#include "ripper.h"


#define MAX_FILENAME_SIZE 100

int main() {
	
	char filename[MAX_FILENAME_SIZE] = "/home/johnson/track1.wav";
	ripper_cd_data_t * rp = ripperInit();
	if(rp->type == AUDIO_CD) {
		printf("Audio CD\n");
		printf("%d Tracks\n",rp->numAudioTracks);
	}
	else if(rp->type == DATA_CD)
		printf("Data CD\n");
	else if(rp->type == MIXED_MODE_CD)
		printf("Mixed Mode CD\n");
	else if(rp->type == NO_CD)
		printf("No CD Found\n");
	
	printf("CD length - %d seconds.\n",rp->cd_length);
	
	int i = 0;
	
	int numMatches = 0;
	ripper_cddb_query_results_t * res = ripperCDDBQuery(rp,&numMatches);
	
	if(res != NULL & numMatches > 0) {
		printf("Artist = %s\n",getRipperCDDBArtist(&res[0]));
		printf("Alblum = %s\n",getRipperCDDBTitle(&res[0]));
		printf("Year = %d\n",getRipperCDDBYear(&res[0]));
		printf("Genre = %s\n",getRipperCDDBGenre(&res[0]));
		printf("Category = %s\n",getRipperCDDBCategory(&res[0]));
		printf("Extended Data = %s\n",getRipperCDDBExtData(&res[0]));
		
		for(i = 1;i <= res[0].numTracks;i++) {
			printf("Track %02d - %s by %s\n",i,getRipperCDDBTrackTitle(&res[0],i),getRipperCDDBTrackArtist(&res[0],i));
		}
	} else {
		printf("Error: No CDDB Record found.\n");
	}

	for(i = 1;i <= res[0].numTracks;++i) 
	{
	  printf("Ripping Track %d: %s\n",i,getRipperCDDBTrackTitle(&res[0],i));
	  ripperRipTrack(rp,i,getRipperCDDBTrackTitle(&res[0],i));
	}

	printf("Complete.\n");
	res = ripperCDDBQueryDestroy(res);
	rp = ripperCDDataDestroy(rp);
	
 	return 0;
}
