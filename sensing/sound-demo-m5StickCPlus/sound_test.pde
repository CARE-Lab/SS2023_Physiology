import processing.serial.*;
import processing.sound.*;

Serial port;
int data;
int x = 0;
SoundFile file;

void setup()
{
  port = new Serial(this, Serial.list()[4], 115200);
  size(300, 300);
  file = new SoundFile(this, "sample.mp3");
  background(0, 0, 0);
  file.play();
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
  text(20,20,data);
  if (data >=95){
    rect(width/2,height/2,width/2,height/2);
    file.play();
  }
  else {
    file.pause();
    circle(150,150,150);
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
