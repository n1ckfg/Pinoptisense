class Locator {
  
  int numSamples = 16;
  ArrayList<PVector> samples;
  PVector cursor;
  float maxDist = 1000;
  
  Locator() {
    samples = new ArrayList<PVector>();
    cursor = new PVector(0,0,0);
  }
  
  Locator(int _numSamples) {
    numSamples = _numSamples;
    samples = new ArrayList<PVector>();
    cursor = new PVector(0,0,0);
  }
  
  void run() {
    float x = dot1.x;
    float y = dot1.y;
    if (samples.size() > 1) {
      PVector lastSample = samples.get(samples.size()-1);
      if (PVector.dist(dot1, lastSample) < maxDist) {
        samples.add(dot1);
      } else {
        clicked = false;
      }
    } else {
      samples.add(dot1);
    }
    if (samples.size() > numSamples) samples.remove(0);
    
    PVector sum = new PVector(0,0,0);
    for (PVector p : samples) {
      sum.add(p);
    }
    cursor = sum.div((float) numSamples);
  }
  
}
