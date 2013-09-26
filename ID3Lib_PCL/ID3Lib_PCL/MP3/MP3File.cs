using System.IO;
using Id3Lib;

namespace Mp3Lib
{
	/// <summary>
	/// Manage MP3 file ID3v2 tags and audio data stream.
	/// </summary>
	public class Mp3File
    {
        #region Fields

        /// <summary>
        /// name of source file
        /// </summary>
        private Stream _sourceStream;

        /// <summary>
        /// contained data for lazy initialise
        /// </summary>
        private Mp3FileData _mp3FileData;

        #endregion

        #region Constructors

       

        /// <summary>
        /// Construct from file info
        /// </summary>
        /// <param name="fileinfo"></param>
        public Mp3File(Stream file)
        {
            _sourceStream = file;
        }

        #endregion

        #region Properties

        /// <summary>
        /// lazy-load the ID3 tags from stream and calculate where the audio must be
        /// </summary>
        /// <remarks>
        /// if this throws an exception, the file is not usable.
        /// </remarks>
        private Mp3FileData Mp3FileData
        {
            get
            {
                if( _mp3FileData == null )
                    _mp3FileData = new Mp3FileData( _sourceStream );

                return _mp3FileData;
            }
        }

       

        /// <summary>
        /// wrapper for the object containing the audio payload
        /// </summary>
        public IAudio Audio
        {
            get { return Mp3FileData.Audio; }
            set { Mp3FileData.Audio = value; }
        }

        /// <summary>
        /// ID3v2 tags represented by the Frame Model
        /// </summary>
        public TagModel TagModel
        {
            get { return Mp3FileData.TagModel; }
            set { Mp3FileData.TagModel = value; }
        }

        /// <summary>
        /// ID3v2 tags wrapped in an interpreter that gives you real properties for each supported frame 
        /// </summary>
        public TagHandler TagHandler
        {
            get { return Mp3FileData.TagHandler; }
            set { Mp3FileData.TagHandler = value; }
        }

        #endregion

        #region Methods
        /// <summary>
        /// Update ID3V2 and V1 tags in-situ if possible, or rewrite the file to add tags if necessary.
        /// </summary>
        public void Update()
        {
            // cheeky optimisation: 
            // if no changes have been made, nothing to do
            if( _mp3FileData != null )
            {
                if( _mp3FileData.Update() == Mp3FileData.CacheDataState.eDirty )
                {
                    // clear the data object so it gets re-initialised next time it's needed
                    _mp3FileData = null;
                }
            }
        }

        /// <summary>
        /// rewrite the file and ID3V2 and V1 tags with no padding.
        /// </summary>
        public void UpdatePacked()
        {
            if( Mp3FileData.UpdatePacked() == Mp3FileData.CacheDataState.eDirty )
            {
                // clear the data object so it gets re-initialised next time it's needed
                _mp3FileData = null;
            }
        }

        /// <summary>
        /// Update file and remove ID3V2 tag (if any); 
        /// update file in-situ if possible, or rewrite the file to remove tag if necessary.
        /// </summary>
        public void UpdateNoV2tag()
        {
            if( Mp3FileData.UpdateNoV2tag() == Mp3FileData.CacheDataState.eDirty )
            {
                // clear the data object so it gets re-initialised next time it's needed
                _mp3FileData = null;
            }
        }

        public string CalculateHash()
        {
            var hash = "";
            //Calculate the hash without ID3 dataù
            try
            {
                _sourceStream.Seek(_mp3FileData.AudioStart, SeekOrigin.Begin);
                byte[] buffer = new byte[this.Audio.NumAudioBytes]; //
                _sourceStream.Read(buffer, 0, (int)this.Audio.NumAudioBytes);
                hash = MD5Core.GetHashString(buffer);
            }
            catch (System.Exception ex)
            {
                int bb = 0;
            }

            return hash;
        }
        #endregion
    }
}
