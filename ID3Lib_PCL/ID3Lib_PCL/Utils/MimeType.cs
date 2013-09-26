using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ID3Lib_PCL
{
    public class MimeType
    {
        private static readonly byte[] BMP = { 66, 77 };
        private static readonly byte[] DOC = { 208, 207, 17, 224, 161, 177, 26, 225 };
        private static readonly byte[] EXE_DLL = { 77, 90 };
        private static readonly byte[] GIF = { 71, 73, 70, 56 };
        private static readonly byte[] ICO = { 0, 0, 1, 0 };
        private static readonly byte[] JPG = { 255, 216, 255 };
        private static readonly byte[] MP3 = { 255, 251, 48 };
        private static readonly byte[] OGG = { 79, 103, 103, 83, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 };
        private static readonly byte[] PDF = { 37, 80, 68, 70, 45, 49, 46 };
        private static readonly byte[] PNG = { 137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82 };
        private static readonly byte[] RAR = { 82, 97, 114, 33, 26, 7, 0 };
        private static readonly byte[] SWF = { 70, 87, 83 };
        private static readonly byte[] TIFF = { 73, 73, 42, 0 };
        private static readonly byte[] TORRENT = { 100, 56, 58, 97, 110, 110, 111, 117, 110, 99, 101 };
        private static readonly byte[] TTF = { 0, 1, 0, 0, 0 };
        private static readonly byte[] WAV_AVI = { 82, 73, 70, 70 };
        private static readonly byte[] WMV_WMA = { 48, 38, 178, 117, 142, 102, 207, 17, 166, 217, 0, 170, 0, 98, 206, 108 };
        private static readonly byte[] ZIP_DOCX = { 80, 75, 3, 4 };

        public static string GetPictureMimeType(byte[] file)
        {
           // string mime = "application/octet-stream"; //DEFAULT UNKNOWN MIME TYPE

            switch(file[0])
            {
                case 0:
                       if (file.Take(4).SequenceEqual(ICO))
                           return "image/x-icon";
                    break;
                case 37:
                    break;
                case 48:
                    break;
                case 66:
                    if (file.Take(2).SequenceEqual(BMP))
                        return "image/bmp";
                    break;
                case 70:
                    break;
                case 71:
                    if (file.Take(4).SequenceEqual(GIF))
                        return "image/gif";
                    break;
                case 73:
                    if (file.Take(4).SequenceEqual(TIFF))
                        return "image/tiff";
                    break;
                case 77:
                    break;
                case 79:
                    break;
                case 80:
                    break;
                case 82:
                    break;
                case 100:
                    break;
                case 137:
                    if (file.Take(16).SequenceEqual(PNG))
                        return "image/png";
                    break;
                case 208:
                    break;
                case 255:
                      if (file.Take(3).SequenceEqual(JPG))
                          return "image/jpeg";
                    break;
            }
            return "image/unknown";
        }
    }
}
