// Copyright(C) 2002-2012 Hugo Rumayor Montemayor, All rights reserved.
using System;
using System.Runtime.Serialization;

namespace Id3Lib.Exceptions
{
    /// <summary>
    /// The exception is thrown when the tag is corrupt.
    /// </summary>
    [DataContract]
    public class InvalidTagException : InvalidStructureException
    {
        //protected InvalidTagException(SerializationInfo info, StreamingContext context)
        //    : base(info, context)
        //{
        //}
        /// <summary>
        /// 
        /// </summary>
        public InvalidTagException()
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="message"></param>
        public InvalidTagException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="message"></param>
        /// <param name="inner"></param>
        public InvalidTagException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}