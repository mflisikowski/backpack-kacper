#include <Adafruit_NeoPixel.h>

// Constants and Pin Definitions
#define PIN_LED 8
#define PIN_BUZZER 6
#define NUM_PIXELS 10

// Sound frequencies
#define FREQ_START 1000
#define FREQ_IDLE 1500
#define FREQ_ERROR 2500

// PWM Configuration
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define PWM_FREQ_BASE 5000

// Volume Configuration
#define VOLUME_MAX 155
#define VOLUME_DEFAULT 64

// Timing Constants
#define MODE_CHANGE_INTERVAL 10000 // 10 seconds
#define ANIMATION_INTERVAL 30      // 30ms
#define STARTUP_STEP_DELAY 10
#define SHUTDOWN_STEP_DELAY 30

// Engine States
enum EngineMode
{
  MODE_IDLE,
  MODE_STARTUP,
  MODE_THRUST,
  MODE_SHUTDOWN,
  MODE_MALFUNCTION
};

Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);

struct Color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class Engine
{
private:
  Adafruit_NeoPixel &pixels;
  uint8_t enginePower;
  EngineMode currentMode;
  unsigned long lastModeChange;
  uint8_t currentVolume;
  bool emergencyShutdown;

  // Add these as private member functions
  void createTurbulence(uint8_t intensity);
  void engineShutdown();
  void simulateMalfunction();

public:
  Engine(Adafruit_NeoPixel &p) : pixels(p),
                                 enginePower(0),
                                 currentMode(MODE_IDLE),
                                 lastModeChange(0),
                                 currentVolume(VOLUME_DEFAULT),
                                 emergencyShutdown(true) {}

  // ... move all engine-related functions here ...

  void engineStartupSequence()
  {
    static unsigned long startTime = 0;
    static uint8_t startupPhase = 0;
    unsigned long currentTime = millis();

    if (startupPhase == 0)
    {
      startTime = currentTime;
      startupPhase++;
      return;
    }

    // Calculate progress based on time
    unsigned long elapsed = currentTime - startTime;

    if (elapsed < 500)
    { // Initial test phase
      // Quick system test logic
    }
    else if (elapsed < 2500)
    { // Power up phase
      uint8_t power = map(elapsed - 500, 0, 2000, 0, 255);
      enginePower = power;
      // Rest of startup logic
    }
    else
    {
      startupPhase = 0;
      currentMode = MODE_THRUST;
    }
  }

  void setVolume(uint8_t vol) { currentVolume = vol; }
  uint8_t getVolume() const { return currentVolume; }
  void adjustVolume(int8_t adjustment);
  void setPower(uint8_t power) { enginePower = power; }
  uint8_t getPower() const { return enginePower; }

  void update()
  {
    unsigned long currentTime = millis();
    if (currentTime - lastModeChange > MODE_CHANGE_INTERVAL && currentMode != MODE_SHUTDOWN)
    {
      lastModeChange = currentTime;
      currentMode = (EngineMode)random(0, 5);
    }

    switch (currentMode)
    {
    case MODE_IDLE:
      pixels.clear();
      pixels.show();
      ledcWriteTone(PWM_CHANNEL, FREQ_IDLE);
      break;
    case MODE_STARTUP:
      engineStartupSequence();
      currentMode = MODE_THRUST;
      break;
    case MODE_THRUST:
      createTurbulence(30);
      setPower(constrain(getPower() + random(-5, 5), 200, 255));
      ledcWriteTone(PWM_CHANNEL, FREQ_IDLE + random(-50, 50));
      break;
    case MODE_SHUTDOWN:
      engineShutdown();
      currentMode = MODE_IDLE;
      break;
    case MODE_MALFUNCTION:
      simulateMalfunction();
      if (random(100) < 5)
        currentMode = MODE_SHUTDOWN;
      break;
    }
    pixels.show();
  }
};

// Global objects
Engine engine(pixels);

// Non-blocking timing variables
unsigned long previousMillis = 0;

class SoundManager
{
private:
  uint8_t currentVolume;

public:
  SoundManager() : currentVolume(VOLUME_DEFAULT) {}

  void playTone(int frequency, int duration);
  void setVolume(uint8_t volume);
  void adjustVolume(int8_t adjustment);
};

class EffectsManager
{
private:
  Adafruit_NeoPixel &pixels;

public:
  EffectsManager(Adafruit_NeoPixel &p) : pixels(p) {}

  void createTurbulence(uint8_t intensity, uint8_t power);
  void showStartupEffect();
  void showShutdownEffect();
  void showMalfunctionEffect(uint8_t type);
};

void setup()
{
  // LED setup
  pixels.begin();
  pixels.setBrightness(100);
  randomSeed(analogRead(0));

  // Sound setup
  ledcSetup(PWM_CHANNEL, PWM_FREQ_BASE, PWM_RESOLUTION);
  ledcAttachPin(PIN_BUZZER, PWM_CHANNEL);
}

// Sound functions
// Funkcja do ustawiania głośności (0-255)
void setVolume(uint8_t volume)
{
  engine.setVolume(volume);
  // Można dodać tutaj EEPROM.write() aby zapamiętać głośność
}

// Funkcja do zmiany głośności relatywnie (+/-)
void adjustVolume(int8_t adjustment)
{
  engine.adjustVolume(adjustment);
}

void playTone(int frequency, int duration)
{
  ledcSetup(PWM_CHANNEL, frequency, PWM_RESOLUTION);
  ledcWrite(PWM_CHANNEL, engine.getVolume());
  delay(duration);
  ledcWrite(PWM_CHANNEL, 0);
}

// Funkcja do odtwarzania dźwięku z określoną głośnością
void playToneWithVolume(int frequency, int duration, uint8_t volume)
{
  uint8_t originalVolume = engine.getVolume();
  engine.setVolume(volume);
  playTone(frequency, duration);
  engine.setVolume(originalVolume);
}

// Original LED functions
Color interpolateColor(Color start, Color end, float progress)
{
  Color result;
  result.r = start.r + (end.r - start.r) * progress;
  result.g = start.g + (end.g - start.g) * progress;
  result.b = start.b + (end.b - start.b) * progress;
  return result;
}

void Engine::createTurbulence(uint8_t intensity)
{
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    int variation = random(-intensity, intensity);
    Color baseColor = {255, (uint8_t)(100 + random(0, 50)), (uint8_t)random(0, 20)};
    float powerFactor = enginePower / 255.0;

    if (i < NUM_PIXELS / 3)
    {
      baseColor.r *= 1.0;
      baseColor.g *= 0.8;
    }
    else if (i < NUM_PIXELS * 2 / 3)
    {
      baseColor.r *= 0.9;
      baseColor.g *= 0.6;
    }
    else
    {
      baseColor.r *= 0.8;
      baseColor.g *= 0.4;
    }

    pixels.setPixelColor(i, pixels.Color(
                                constrain(baseColor.r + variation, 0, 255) * powerFactor,
                                constrain(baseColor.g + variation, 0, 255) * powerFactor,
                                constrain(baseColor.b + variation, 0, 255) * powerFactor));
  }
}

void Engine::engineShutdown()
{
  while (enginePower > 0)
  {
    enginePower = constrain((int)enginePower - 5, 0, 255);

    // Decreasing sound frequency
    ledcWriteTone(PWM_CHANNEL, FREQ_START * enginePower / 255);

    createTurbulence(map(enginePower, 0, 255, 5, 50));

    for (int i = 0; i < NUM_PIXELS; i++)
    {
      Color current = {
          enginePower,
          (uint8_t)(enginePower * 0.3),
          0};
      pixels.setPixelColor(i, pixels.Color(current.r, current.g, current.b));
    }

    pixels.show();
    delay(30);
  }
  ledcWriteTone(PWM_CHANNEL, 0);
}

void Engine::simulateMalfunction()
{
  static uint8_t malfunctionType = random(0, 3);

  // Play error sound
  ledcWriteTone(PWM_CHANNEL, FREQ_ERROR + random(-200, 200));

  switch (malfunctionType)
  {
  case 0: // Unstable combustion
    createTurbulence(100);
    enginePower = constrain((int)enginePower - random(0, 10), 0, 255);
    break;

  case 1: // Pulsations
    enginePower = constrain((int)enginePower + sin(millis() / 100.0) * 50, 0, 255);
    createTurbulence(50);
    break;

  case 2: // Asymmetric thrust
    for (int i = 0; i < NUM_PIXELS; i++)
    {
      if (i % 2 == 0)
      {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
    }
    break;
  }
}

void loop()
{
  unsigned long currentMillis = millis();

  // Non-blocking timing for main loop
  if (currentMillis - previousMillis >= ANIMATION_INTERVAL)
  {
    previousMillis = currentMillis;
    engine.update();
  }
}