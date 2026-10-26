#pragma once

void audioBegin();
bool audioPlay(const char* path);  // false if the file could not be opened
void audioLoop();                  // call every loop() iteration
bool audioRunning();               // false once the track ends or is stopped
void audioStop();
