import { KeyPoint, Contour, imshow, cvtColor, COLOR_RGB2GRAY, drawKeypoints, countNonZero, findHomography, perspectiveTransform, drawLine, drawCircle, VideoCapture, BriefDescriptorExtractor, BFMatcher, FastFeatureDetector, CV_32FC1, Mat, DRAW_OVER_OUTIMG, Scalar, RANSAC, waitKey, } from 'opencv';

/*static void help(char **av)
{
console.log("\nThis program demonstrated the use of features2d with the Fast corner detector and brief descriptors\n"
    << "to track planar objects by computing their homography from the key (training) image to the query (test) image\n\n");
console.log("usage: " << av[0] << " <video device number>\n");
console.log("The following keys do stuff:");
console.log("  t : grabs a reference frame to match against");
console.log("  l : makes the reference frame new every frame");
console.log("  q or escape: quit");
}*/

function drawMatchesRelative(train, query, matches, img, mask) {
  for(let i = 0; i < matches.length; i++) {
    if(mask.empty || mask[i]) {
      const pt_new = query[matches[i].queryIdx].pt;
      const pt_old = train[matches[i].trainIdx].pt;

      drawLine(img, pt_new, pt_old, Scalar(125, 255, 125), 1);
      drawCircle(img, pt_new, 2, Scalar(255, 0, 125), 1);
    }
  }
}

//Takes a descriptor and turns it into an xy point
function keypoints2points(inp, out) {
  out.splice(0, out.length);

  for(let i = 0; i < inp.length; ++i) {
    out.push(inp[i].pt);
  }
}

//Takes an xy point and appends that to a keypoint structure
function points2keypoints(inp, out) {
  out.splice(0, out.length);

  for(let i = 0; i < inp.length; ++i) {
    out.push(new KeyPoint(inp[i], 1));
  }
}

//Uses computed homography H to warp original input points to new planar position
function warpKeypoints(H, inp, out) {
  const pts = new Contour();

  keypoints2points(inp, pts);

  const pts_w = new Contour(pts.length);
  const m_pts_w = new Mat(pts_w);

  perspectiveTransform(new Mat(pts), m_pts_w, H);

  points2keypoints(pts_w, out);
}

//Converts matching indices to xy points
function matches2points(train, query, matches, pts_train, pts_query) {
  pts_train.splice(0, pts_train.length);
  pts_query.splice(0, pts_query.length);

  for(let i = 0; i < matches.length; i++) {
    const dmatch = matches[i];

    pts_query.push(query[dmatch.queryIdx].pt);
    pts_train.push(train[dmatch.trainIdx].pt);
  }
}

function main(cam = 0) {
  const brief = new BriefDescriptorExtractor(32);

  const capture = new VideoCapture();

  capture.open(cam);

  if(!capture.isOpened()) {
    //help(av);
    console.log('capture device ' + args[1] + ' failed to open!');
    return 1;
  }

  console.log('following keys do stuff:');
  console.log('t : grabs a reference frame to match against');
  console.log('l : makes the reference frame new every frame');
  console.log('q or escape: quit');

  let frame = new Mat();

  const matches = [];

  const desc_matcher = new BFMatcher(brief.defaultNorm);

  let train_pts = new Contour(),
    query_pts = new Contour();
  let train_kpts = [],
    query_kpts = [];
  /*vector<unsigned char>*/ let match_mask = new Mat();

  const gray = new Mat();

  let ref_live = true;

  let train_desc = new Mat(),
    query_desc = new Mat();
  const detector = new FastFeatureDetector(10, true);

  let H_prev = Mat.eye(3, 3, CV_32FC1);

  function resetH() {
    H_prev = Mat.eye(3, 3, CV_32FC1);
  }

  for(;;) {
    capture.read(frame);

    if(frame.empty) break;

    cvtColor(frame, gray, COLOR_RGB2GRAY);

    detector.detect(gray, query_kpts); //Find interest points

    //console.log('keypoints', query_kpts.length);

    brief.compute(gray, query_kpts, query_desc); //Compute brief descriptors at each keypoint location

    //console.log('query_desc.rows', query_desc.rows);

    if(train_kpts.length) {
      const test_kpts = [];

      warpKeypoints(H_prev.inv(), query_kpts, test_kpts);

      //console.log('test_kpts', test_kpts);

      //Mat mask = windowedMatchingMask(test_kpts, train_kpts, 25, 25);
      desc_matcher.match(query_desc, train_desc, matches, new Mat());

      console.log('matches.length', matches.length);

      drawKeypoints(frame, test_kpts, frame, Scalar(255, 0, 0), DRAW_OVER_OUTIMG);

      matches2points(train_kpts, query_kpts, matches, train_pts, query_pts);

      if(matches.length > 5) {
        /*console.log('train_pts', train_pts.length);
        console.log('query_pts', query_pts.length);*/

        let H = findHomography(train_pts, query_pts, RANSAC, 4, match_mask);

        const nz = countNonZero(  match_mask);

        //console.log('nz', nz);

        if(nz > 15) H_prev = H;
        else resetH();

        drawMatchesRelative(train_kpts, query_kpts, matches, frame, match_mask);
      } else {
        resetH();
      }
    } else {
      resetH();

      let out = new Mat();
      drawKeypoints(gray, query_kpts, out);
      frame = out;
    }

    imshow('frame', frame);

    if(ref_live) {
      train_kpts = query_kpts;
      query_desc.copyTo(train_desc);
    }

    let key = waitKey(2);

    //console.log('key', key);

    switch (key) {
      case 'l':
      case 108:
        ref_live = true;
        resetH();
        break;

      case 't':
      case 116:
        ref_live = false;
        train_kpts = query_kpts;
        query_desc.copyTo(train_desc);
        resetH();
        break;

      case 27:
      case 113:
      case 'q':
        return 0;
        break;
    }
  }
  return 0;
}

try {
  main(...scriptArgs.slice(1));
} catch(e) {
  console.log('error', e);
}
