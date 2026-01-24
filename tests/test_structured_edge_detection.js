import { ximgproc, imread, Mat, CV_32FC1, imshow, waitKey } from 'opencv';

function main(filename = 'tests/test_linesegmentdetector.jpg') {
  const src = imread(filename);

  if(src.empty) throw new Error(`Error opening image: ${filename}`);

  const pDollar = ximgproc.createStructuredEdgeDetection('tests/model.yml.gz');

  const image = new Mat();
  src.convertTo(image, CV_32FC1, 1 / 255.0);

  const edges = new Mat();
  pDollar.detectEdges(image, edges);

  const orientation_map = new Mat();
  pDollar.computeOrientation(edges, orientation_map);

  const edge_nms = new Mat();
  pDollar.edgesNms(edges, orientation_map, edge_nms, 2, 0, 1, true);

  imshow('edges', edges);
  imshow('edges nms', edge_nms);
  waitKey(-1);
}

main(...scriptArgs.slice(1));
