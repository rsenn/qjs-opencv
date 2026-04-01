import { CommandLineParser, Mat, VideoCapture, CAP_ANY, CAP_PROP_AUDIO_BASE_INDEX, CAP_PROP_AUDIO_DATA_DEPTH, CAP_PROP_AUDIO_SAMPLES_PER_SECOND, CAP_PROP_AUDIO_STREAM, CAP_PROP_AUDIO_TOTAL_CHANNELS, CAP_PROP_AUDIO_TOTAL_STREAMS, CAP_PROP_VIDEO_STREAM, waitKey, imshow, depthToString, CV_16S, } from 'opencv';

function main(...argv) {
  const parser = new CommandLineParser(scriptArgs, '{@audio||}');
  const file = parser.get('@audio');

  if(file == '') {
    return 1;
  }

  const videoFrame = new Mat();
  const audioFrame = new Mat();
  const audioData = [];
  const cap = new VideoCapture();
  const params = [CAP_PROP_AUDIO_STREAM, 0, CAP_PROP_VIDEO_STREAM, 0, CAP_PROP_AUDIO_DATA_DEPTH, CV_16S];

  cap.open(file, CAP_ANY, params);
  if(!cap.isOpened()) {
    console.log("ERROR! Can't to open file: " + file);
    return -1;
  }

  const audioBaseIndex = cap.get(CAP_PROP_AUDIO_BASE_INDEX);
  const numberOfChannels = cap.get(CAP_PROP_AUDIO_TOTAL_CHANNELS);
  console.log('CAP_PROP_AUDIO_DATA_DEPTH: ' + depthToString(cap.get(CAP_PROP_AUDIO_DATA_DEPTH)));
  console.log('CAP_PROP_AUDIO_SAMPLES_PER_SECOND: ' + cap.get(CAP_PROP_AUDIO_SAMPLES_PER_SECOND));
  console.log('CAP_PROP_AUDIO_TOTAL_CHANNELS: ' + cap.get(CAP_PROP_AUDIO_TOTAL_CHANNELS));
  console.log('CAP_PROP_AUDIO_TOTAL_STREAMS: ' + cap.get(CAP_PROP_AUDIO_TOTAL_STREAMS));

  for(let i = 0; i < numberOfChannels; i++) audioData.push([]);

  let numberOfSamples = 0;
  let numberOfFrames = 0;

  //audioData.resize(numberOfChannels);

  for(;;) {
    if(cap.grab()) {
      cap.retrieve(videoFrame);

      for(let nCh = 0; nCh < numberOfChannels; nCh++) {
        cap.retrieve(audioFrame, audioBaseIndex + nCh);

        if(!audioFrame.empty) audioData[nCh].push(audioFrame);
        numberOfSamples += audioFrame.cols;
        console.log('Number of audio samples: ' + numberOfSamples);
      }

      if(false && !videoFrame.empty) {
        numberOfFrames++;

        imshow('Live', videoFrame);

        if(waitKey(5) >= 0) break;
      }
    } else {
      break;
    }
  }

  console.log('Number of audio samples: ' + numberOfSamples);
  console.log('Number of video frames: ' + numberOfFrames);
  return 0;
}

try {
  main(...scriptArgs.slice(1));
} catch(e) {
  console.log('error', e);
}
