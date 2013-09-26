using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;

namespace Mp3Lib
{
    class AudioFile : Audio
    {
        #region Fields

        /// <summary>
        /// holds audio stream filename; opened afresh when we need the data
        /// </summary>
        private Stream _sourceStream;
        /// <summary>
        /// offset from start of stream that the audio starts at
        /// </summary>
        private UInt32 _payloadStart;
        /// <summary>
        /// total length (bytes) of mp3 audio frames in the file,
        /// could be different from what's declared in the header if the file is corrupt.
        /// </summary>
        private UInt32 _payloadNumBytes;

        #endregion

        #region Construction

        /// <summary>
        /// construct audio file
        /// passing in audio size and id3 length tag (if any) to help with bitrate calculations
        /// </summary>
        /// <param name="sourceStream"></param>
        /// <param name="audioStart"></param>
        /// <param name="payloadNumBytes"></param>
        /// <param name="id3DurationTag"></param>
        public AudioFile(Stream sourceStream, UInt32 audioStart, UInt32 payloadNumBytes, TimeSpan? id3DurationTag)
            :
                base(ReadFirstFrame(sourceStream, audioStart, payloadNumBytes), id3DurationTag)
        {
            _sourceStream = sourceStream;
            _payloadStart = audioStart;
            _payloadNumBytes = payloadNumBytes;

            CheckConsistency();
        }

        private static AudioFrame ReadFirstFrame(Stream sourceStream, UInt32 audioStart, UInt32 audioNumBytes)
        {
            sourceStream.Seek(audioStart, SeekOrigin.Begin);

            // read a base level mp3 frame header
            // if it can't even do that right, just fail the call.
            return AudioFrameFactory.CreateFrame(sourceStream, audioNumBytes);
        }

        #endregion

        #region Properties

        /// <summary>
        /// text info, e.g. the encoding standard of audio data in AudioStream
        /// /// </summary>
        public override string DebugString
        {
            get
            {
                //----AudioFile----
                //  Header starts: 12288 bytes
                //  FileSize: 4750766 bytes
                string retval = string.Format("{0}\n----AudioFile----\n  Header starts: {1} bytes\n  Payload: {2} bytes",
                                              base.DebugString,
                                              _payloadStart,
                                              _payloadNumBytes);
                return retval;
            }
        }

        /// <summary>
        /// the number of bytes of data in AudioStream, always the real size of the file
        /// </summary>
        public override uint NumPayloadBytes
        {
            get
            {
                return _payloadNumBytes;
            }
        }

        #endregion

        #region IAudio Functions

        /// <summary>
        /// the stream containing the audio data, wound to the start
        /// </summary>
        /// <remarks>
        /// it is the caller's responsibility to dispose of the returned stream
        /// and to call NumPayloadBytes to know how many bytes to read.
        /// </remarks>
        public override Stream OpenAudioStream()
        {
            _sourceStream.Seek(_payloadStart, SeekOrigin.Begin);
            return _sourceStream;
        }

        /// <summary>
        /// calculate sha-1 of the audio data
        /// </summary>
        //public override byte[] CalculateAudioSHA1()
        //{
        //    using (Stream stream = OpenAudioStream())
        //    {
        //        // This is one implementation of the abstract class SHA1.
        //        System.Security.Cryptography.SHA1 sha = new System.Security.Cryptography.SHA1CryptoServiceProvider();

        //        uint numLeft = _payloadNumBytes;

        //        const int size = 4096;
        //        byte[] bytes = new byte[4096];
        //        int numBytes;

        //        while (numLeft > 0)
        //        {
        //            // read a whole block, or to the end of the file
        //            numBytes = stream.Read(bytes, 0, size);

        //            // audio ends on or before end of this read; exit loop and checksum what we have.
        //            if (numLeft <= numBytes)
        //                break;

        //            sha.TransformBlock(bytes, 0, size, bytes, 0);
        //            numLeft -= (uint)numBytes;
        //        }

        //        sha.TransformFinalBlock(bytes, 0, (int)numLeft);

        //        byte[] result = sha.Hash;
        //        return result;
        //    }
        //}

        public override string CalculateHash()
        {
            byte[] buffer = new byte[_sourceStream.Length];
            _sourceStream.Seek(0, SeekOrigin.Begin);
            _sourceStream.Read(buffer, 0,(int) _sourceStream.Length);
            return MD5Core.GetHashString(buffer);
        }

        #endregion
    }
}
