import peasy.PeasyCam;

PeasyCam cam;

float dotSize = 20;
ArrayList<Stroke> strokes;

void setup() {
  size(640, 480, P3D);
  cam =  new PeasyCam(this, width/2, height/2, 400, 50);

  oscSetup();
  strokes = new ArrayList<Stroke>();
}

void draw() {
  background(0);
  lights();

  ArrayList<Point> pointsToAdd = new ArrayList<Point>();

  for (int i=0; i<maxDots; i++) {
    boolean doDot1 = false;
    if (dot1[i] != null) doDot1 = true;
    
    stroke(63);
    strokeWeight(1);
    if (doDot1) {
      line(0, dot1[i].y, dot1[i].x, dot1[i].y);
      line(dot1[i].x, 0, dot1[i].x, dot1[i].y);
    }
    
    stroke(255);    
    strokeWeight(8);
    if (doDot1) {
      stroke(0,127,255);
      point(dot1[i].x, dot1[i].y);
    }
    
    stroke(255);
    strokeWeight(2);
    fill(127);
    ellipseMode(CENTER);
    if (doDot1) ellipse(dot1[i].x, dot1[i].y, dotSize, dotSize);
  }
  
  if (pointsToAdd.size() > strokes.size()) {
    for (int i=strokes.size(); i<pointsToAdd.size(); i++) {
      strokes.add(new Stroke());
    }
  }
  
  for (int i=0; i<pointsToAdd.size(); i++) {
    strokes.get(i).addPoint(pointsToAdd.get(i));
  }
  
  for (int i=0; i<strokes.size(); i++) {
    strokes.get(i).run();
  }
  
  surface.setTitle("" + frameRate);
}
