#include <TM1637.h>

// Joysticks pins
const int player1Pin = A0;
const int player2Pin = A15;
// Button pin
const int buttonPin = 8;
// Player LED Pins
const int player1LEDPin = 12;
const int player2LEDPin = 2;
// Buzzer pin
const int buzzerPin = 6;
enum { WILL_PLAY = 0, PLAYING = 1, STOP = 2 };
int buzzerTimer = 0;
int buzzerState = STOP;
int winSoundPlayed = 0;
// Score display Pin
int DIO = 31;
int CLK = 30;
TM1637 tm(CLK, DIO);
// ball speed
unsigned long defaultRefreshDelay = 20; // default to 20ms
unsigned long lastRefreshedDelay = 0;
unsigned long refreshDelay = defaultRefreshDelay;
// game state
enum { STARTING = 1, IN_PROGRESS = 2, P1_WON = 3, P2_WON = 4 };
int gameState = STARTING;
int player1Score = 0;
int player2Score = 0;
int maxScore = 5;
// ball state
enum { FREEZE = 0, LB = 1, LT = 2, RB = 3, RT = 4}; // Ball froze, or bottom left, top left, bottom right, top right
int ballState = LB;
int player1Position[2] = {21, 29};
int player2Position[2] = {21, 29};
int ballPosition[2] = {35, 25};

/**
 * @name setup
 * @description setup our pins
 */
void setup() {
  Serial.begin(57600);
  // setup player joystick
  pinMode(player1Pin, INPUT);
  pinMode(player2Pin, INPUT);
  // setup LED Pins
  pinMode(player1LEDPin, OUTPUT);
  pinMode(player2LEDPin, OUTPUT);
  // setup button
  pinMode(buttonPin, INPUT);
  // setup buzzer
  pinMode(buzzerPin, OUTPUT);
  // initialize digit display
  tm.init();
  tm.set(7);
}

/**
 * @name printMessage
 * @description print message to web interface, equivalent to JS console.log
 */
void printMessage(String message) {
  String msg = "{\"message\": \"" + message + "\"} \n";
  Serial.print(msg);
}

/**
 * @name handlePlayersMovement
 * @description update player position depending on their analog sticks
 */
void handlePlayersMovement() {
  // handle players next position
  int player1Direction = analogRead(player1Pin);
  // if player 1 go down
  if (player1Direction < 300 && player1Position[0] > 0) {
    player1Position[0] = player1Position[0] - 1;
    player1Position[1] = player1Position[1] - 1;
    // else if player 1 go up
  } else if (player1Direction > 700 && player1Position[1] < 50) {
    player1Position[0] = player1Position[0] + 1;
    player1Position[1] = player1Position[1] + 1;
  }
  int player2Direction = analogRead(player2Pin);
  // if player 2 go down
  if (player2Direction < 300 && player2Position[0] > 0) {
    player2Position[0] = player2Position[0] - 1;
    player2Position[1] = player2Position[1] - 1;
    // else if player 2 go up
  } else if (player2Direction > 700 && player2Position[1] < 50) {
    player2Position[0] = player2Position[0] + 1;
    player2Position[1] = player2Position[1] + 1;
  }
}

/**
 * @name handleBallMovement
 * @description update ball position depending on previous one
 */
void handleBallMovement() {
  // handle ball next position
  if (ballState == LB) {
    ballPosition[0] = ballPosition[0] - 1;
    ballPosition[1] = ballPosition[1] - 1;
    digitalWrite(player1LEDPin, HIGH);
    digitalWrite(player2LEDPin, LOW);
    
  } else if (ballState == LT) {
    ballPosition[0] = ballPosition[0] - 1;
    ballPosition[1] = ballPosition[1] + 1;
    digitalWrite(player1LEDPin, HIGH);
    digitalWrite(player2LEDPin, LOW);
  }  else if (ballState == RB) {
    ballPosition[0] = ballPosition[0] + 1;
    ballPosition[1] = ballPosition[1] - 1;
    digitalWrite(player1LEDPin, LOW);
    digitalWrite(player2LEDPin, HIGH);
  }  else if (ballState == RT) {
    ballPosition[0] = ballPosition[0] + 1;
    ballPosition[1] = ballPosition[1] + 1;
    digitalWrite(player1LEDPin, LOW);
    digitalWrite(player2LEDPin, HIGH);
  }
}

/**
 * @name sendData
 * @description send data to node APP in a JSON format
 */
void sendData() {
  String data = "";
  data = data + "{ \"ballPosition\": [" + ballPosition[0] + "," + ballPosition[1] + "]";
  data = data + ", \"player1Position\": [" + player1Position[0] + "," + player1Position[1] + "]";
  data = data + ", \"player2Position\": [" + player2Position[0] + "," + player2Position[1] + "]";
  data = data + ", \"gameState\": " + gameState;
  data = data + "} \n";
  Serial.print(data);
}

/**
 * @name handleBallInteraction
 * @description handle if a ball reach any wall, or a player
 */
void handleBallInteraction() {
  int randomValue = random(0,2); // random generator to know if ball is going to top or bottom
  // handle if ball reached a border
  if(ballPosition[0] == 0) { // Player 2 scores
    ballPosition[0] = 35;
    ballPosition[1] = 25;
    ballState = randomValue == 1 ? RT : RB;
    player2Score = player2Score + 1;
    refreshDelay = defaultRefreshDelay; // reset refresh delay
    printMessage("PLAYER 2 SCORE");
    buzzerState = WILL_PLAY;
    return;
  } else if (ballPosition[0] == 70) { // Player 1 scores
    ballPosition[0] = 35;
    ballPosition[1] = 25;
    ballState = randomValue == 1 ? LT : LB;
    player1Score = player1Score + 1;
    refreshDelay = defaultRefreshDelay; // reset refresh delay
    buzzerState = WILL_PLAY;
    printMessage("PLAYER 1 SCORE");
    return;
  }
  // handle if ball touched a top or bottom wall
  if (ballPosition[1] == 0) {
    ballState = ballState == LB ? LT : RT;
    buzzerState = WILL_PLAY;
    printMessage("TOUCHED BOTTOM WALL");
    return;
  } else if (ballPosition[1] == 50) {
    ballState = ballState == LT ? LB : RB;
    buzzerState = WILL_PLAY;
    printMessage("TOUCHED TOP WALL");
    return;
  }
  // handle if ball is touched by a player
  if (ballPosition[0] == 2 && ballPosition[1] >= player1Position[0] && ballPosition[1] <= player1Position[1]) {
    //touched by player 1
    ballState = randomValue == 1 ? RT : RB;
    // make game faster after a hit
    refreshDelay = refreshDelay > 0 ? refreshDelay - 2 : refreshDelay;
    buzzerState = WILL_PLAY;
    return;
  } else if (ballPosition[0] == 68 && ballPosition[1] >= player2Position[0] && ballPosition[1] <= player2Position[1]) {
    //touched by player 2
    // make game faster after a hit
    refreshDelay = refreshDelay > 0 ? refreshDelay - 2 : refreshDelay;
    ballState = randomValue == 1 ? LT : LB;
    buzzerState = WILL_PLAY;
    return;
  }
}

/**
 * @name playWin
 * @description play win song
 */
void playWin() {
  tone(buzzerPin, 330);
  delay(600);
  noTone(buzzerPin);
  delay(50);
  
  tone(buzzerPin, 330);
  delay(200);
  noTone(buzzerPin);
  delay(50);
  tone(buzzerPin, 370);
  delay(400);
  noTone(buzzerPin);
  delay(50);
  
  tone(buzzerPin, 415);
  delay(400);
  noTone(buzzerPin);
  delay(50);
  
  tone(buzzerPin, 440);
  delay(400);
  noTone(buzzerPin);
}

/**
 * @name handleGameEnd
 * @description Check if game ends depending on players score
 */
void handleGameEnd() {
  if(player1Score == maxScore){
    gameState = P1_WON;
    digitalWrite(player1LEDPin, HIGH);
    digitalWrite(player2LEDPin, LOW);
  } else if (player2Score == maxScore) {
    gameState = P2_WON;
    digitalWrite(player1LEDPin, LOW);
    digitalWrite(player2LEDPin, HIGH);
  }
}

/**
 * @name handleBuzzer
 * @description handle if buzzer must be set or shutdown
 */
void handleBuzzer() {
  int timeRemaining = 50 - (millis() - buzzerTimer);
  if(buzzerState == WILL_PLAY) {
    tone(buzzerPin, 112);
    buzzerTimer = millis();
    buzzerState = PLAYING;
  } else if (timeRemaining <= 0 && buzzerState == PLAYING) {
    noTone(buzzerPin);
    buzzerState = STOP;
  }
}

/**
 * @name displayScore
 * @description display sore in our digits output
 */
void displayScore() {
  tm.display(0, 0);
  tm.display(1, player1Score);
  tm.display(2, 0);
  tm.display(3, player2Score);
}

/**
 * @name resetGame
 * @description reset the game values
 */
void(* resetGame) (void) = 0;  // declare reset fuction at address 0

void loop() {
  int timeBeforeRefresh = refreshDelay - (millis() - lastRefreshedDelay);
  int buttonPressed = digitalRead(buttonPin);
  if(gameState == IN_PROGRESS) {
      handlePlayersMovement();
      if (timeBeforeRefresh <= 0) {
      lastRefreshedDelay = millis();
      handleBallMovement();
      handleBallInteraction();
    }
      displayScore();
      handleBuzzer();
      sendData();
      handleGameEnd();
    } else if (gameState == STARTING) {
      if (buttonPressed == HIGH) {
        gameState = IN_PROGRESS;
      }
      // display start message
      sendData();
    } else {
      if (buttonPressed == HIGH) {
        delay(400);
        resetGame();
      }
      // display winner message
      sendData();
      handleBuzzer();
      // handle win sound
      if(winSoundPlayed == 0) {
        winSoundPlayed = 1;
        playWin();
      }
    }
}
