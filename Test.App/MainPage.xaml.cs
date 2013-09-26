using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using Test.App.Resources;
using Windows.Storage;
using System.IO;
using Mp3Lib;
using System.Windows.Data;
using Id3Lib.Frames;
using System.Windows.Media.Imaging;

namespace Test.App
{
    public partial class MainPage : PhoneApplicationPage
    {
        LameDecoder.DecoderComponent wc;
        // Constructor
        public MainPage()
        {
            InitializeComponent();

            // Sample code to localize the ApplicationBar
            //BuildLocalizedApplicationBar();
            wc = new LameDecoder.DecoderComponent();

            Binding myBinding = new Binding("NoVoice");
            myBinding.Source = wc;
            myBinding.Mode = BindingMode.TwoWay;
            BindingOperations.SetBinding(chkNoVoice, CheckBox.IsCheckedProperty, myBinding);


        }

        Stream audio = null;
        int FrameLengthInBytes;
        int Position;
        async private void Button_Click(object sender, RoutedEventArgs e)
        {
            if ((string)btnPlay.Content == "Play")
            {
                var folder = Windows.ApplicationModel.Package.Current.InstalledLocation;
                var file = await folder.GetFileAsync(@"assets\eraserewind.mp3");
                var stream = await file.OpenAsync(Windows.Storage.FileAccessMode.Read);
                try
                {
                    Mp3File mp3 = new Mp3File(stream.AsStreamForRead());
                    FrameLengthInBytes = 10 * (int)mp3.Audio.Header.FrameLengthInBytes;
                    audio = mp3.Audio.OpenAudioStream();
                    Position = 0;

                    if (mp3.TagModel.Count > 0)
                    {
                        lblTag1.Text = mp3.TagHandler.Artist;
                        lblTag2.Text = mp3.TagHandler.Song;
                        lblTag3.Text = mp3.TagHandler.Album;

                        for(int i=0;i<mp3.TagModel.Count;i++)
                            if (mp3.TagModel[i].FrameId == "APIC") //picture
                            {
                                MemoryStream ms = new MemoryStream();
                                ms.Write(((FramePicture)mp3.TagModel[i]).PictureData, 0, ((FramePicture)mp3.TagModel[i]).PictureData.Length);

                                var imageSource = new BitmapImage();
                                imageSource.SetSource( ms);

                                // Assign the Source property of your image
                                imgTag.Source = imageSource;
                            }
                    }
                    

                    byte[] bytes = new byte[audio.Length];
                    audio.Read(bytes, 0, (int)audio.Length);
                    if (wc.Play(bytes, mp3.Audio.Header.IsMono ? 1 : 2, (int)mp3.Audio.Header.SamplesPerSecond, (int)mp3.Audio.Header.BitRate))
                        btnPlay.Content = "Stop";
                }
                catch
                {
                }
            }
            else
            {
                wc.Stop();
                btnPlay.Content = "Play";
                imgTag.Source = null;
                lblTag1.Text = "";
                lblTag2.Text = "";
                lblTag3.Text = "";

            }
            //wc.SetBytestream(stream);
            //wc.SetBytestream("eraserewind.mp3");

        }

        // Sample code for building a localized ApplicationBar
        //private void BuildLocalizedApplicationBar()
        //{
        //    // Set the page's ApplicationBar to a new instance of ApplicationBar.
        //    ApplicationBar = new ApplicationBar();

        //    // Create a new button and set the text value to the localized string from AppResources.
        //    ApplicationBarIconButton appBarButton = new ApplicationBarIconButton(new Uri("/Assets/AppBar/appbar.add.rest.png", UriKind.Relative));
        //    appBarButton.Text = AppResources.AppBarButtonText;
        //    ApplicationBar.Buttons.Add(appBarButton);

        //    // Create a new menu item with the localized string from AppResources.
        //    ApplicationBarMenuItem appBarMenuItem = new ApplicationBarMenuItem(AppResources.AppBarMenuItemText);
        //    ApplicationBar.MenuItems.Add(appBarMenuItem);
        //}
    }
}