
#ifndef RIPPER_ENGINE_HPP_
#define RIPPER_ENGINE_HPP_

#include <sys/types.h>

namespace SimpleRipper
{
  // Forward Declarations
  class CDDrive;
  class AudioDrive;
  class AudioTrack;

  //! Abstract class for ripping audo tracks from a cd
  class RipperEngine
  {
    public:
      //! Type of cd in the drive
      typedef enum
      {
	AUDIO_CD,
	DATA_CD,
	MIXED_MODE_CD,
	NO_CD
      } CDType;      
      
    public:
      
      RipperEngine
	(
	CDDrive & aDrive
	);
      
      virtual RipperEngine();
      
      virtual AudioTrack * ripTrack
	(
	uint aTrackNum
	int * aProgress,
	bool * aCancel
	) = 0;
	
      inline void setDrive
	(
	CDDrive * aDrive	//!< Pointer to the cd drive to use
	)
      {
	mDrive = aDrive;
      }	  
	
      inline const CDDrive & getDrive() const { return mDrive; }
      
      inline CDType getCDType() const { return mType; }
      
    protected:
      
      CDType	&mType;		//!< CD Type - Audio, Mixed Mode, etc	
      CDDrive 	*mDrive;	//!< Pointer to the cd drive in use
      
	
  };

} //end namespace SimpleRipper

#endif