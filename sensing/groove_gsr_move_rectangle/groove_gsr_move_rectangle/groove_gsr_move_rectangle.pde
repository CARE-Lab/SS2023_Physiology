import processing.serial.*;

Serial port;
int data;
int x = 0;

void setup() 
{
  port = new Serial(this, Serial.list()[0], 115200);
  size(300, 300);
  background(0, 0, 0);
}

void draw() 
{
  // 描画エリア設定
  fill(255);
  noStroke();
  rect(0, 0, width, height);
  fill(0);
  textSize(height*0.10);
  textAlign(LEFT);
  if (data >=50){
    x += 1;
    rect(width/2+x, height/2, 15,15);
    fill(255);
    text(data, width*0.5, height*0.3);
    fill(255);

  }
  else {
    x -= 1;
    rect(width/2+x, height/2, 15,15);
    fill(255);
    text(data, width*0.5, height*0.3);
    fill(255);
  }
}

void serialEvent(Serial port)
{
  // シリアルポートからデータを受け取ったら
  if (port.available() >=1) 
  {
    data = port.read();
  }
}
