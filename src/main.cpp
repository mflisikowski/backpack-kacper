#include <Adafruit_NeoPixel.h>

// Definicje pinów
#define LED_PIN 20   // 8   // Pin do którego podłączone są LEDy
#define BUZZER_PIN 6 // Pin do którego podłączony jest buzzer
#define NUM_LEDS 16  // Liczba LEDów w pasku

// Utworzenie obiektu NeoPixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Kolory używane w animacji
const uint32_t FLAME_COLORS[] = {
    strip.Color(255, 0, 0),   // Czerwony
    strip.Color(255, 69, 0),  // Pomarańczowo-czerwony
    strip.Color(255, 140, 0), // Ciemnopomarańczowy
    strip.Color(255, 165, 0), // Pomarańczowy
    strip.Color(255, 200, 0), // Jasnopomarańczowy
    strip.Color(255, 220, 50) // Żółto-pomarańczowy
};
const int NUM_COLORS = sizeof(FLAME_COLORS) / sizeof(FLAME_COLORS[0]);

// Częstotliwości dźwięków dla efektu startowego
const int SOUND_FREQUENCIES[] = {440, 880, 1320, 2200, 2640};

// Add these constants at the top of the file
const float NASA_ENGINE_FREQUENCY = 3000.0f; // Hz
const float NASA_ENGINE_AMPLITUDE = 0.8f;    // 0.0 to 1.0
const float NASA_RUMBLE_FREQUENCY = 100.0f;  // Low frequency rumble
const float SAMPLE_RATE = 44100.0f;          // Standard audio sample rate

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class NASAEngineSound
{
private:
  float currentPhase = 0.0f;
  float rumblePhase = 0.0f;
  float currentFrequency = NASA_ENGINE_FREQUENCY;
  float currentAmplitude = NASA_ENGINE_AMPLITUDE;

public:
  float generateSample(float deltaTime)
  {
    // Main engine whine
    float mainSound = sin(2.0f * M_PI * currentFrequency * currentPhase);

    // Low frequency rumble
    float rumble = sin(2.0f * M_PI * NASA_RUMBLE_FREQUENCY * rumblePhase);

    // Combine sounds with different weights
    float combinedSound = (mainSound * 0.7f + rumble * 0.3f) * currentAmplitude;

    // Update phases
    currentPhase += deltaTime;
    rumblePhase += deltaTime;

    // Wrap phases to prevent floating point precision issues
    if (currentPhase > 1.0f)
      currentPhase -= 1.0f;
    if (rumblePhase > 1.0f)
      rumblePhase -= 1.0f;

    return combinedSound;
  }

  void setThrottle(float throttleLevel)
  {
    // Adjust frequency based on throttle (0.0 to 1.0)
    currentFrequency = 3000.0f + (throttleLevel * 2000.0f);
    currentAmplitude = 0.3f + (throttleLevel * 0.5f);
  }
};

// Dodajemy nowe stałe dla dźwięków
const int COUNTDOWN_BEEP = 880; // Dźwięk odliczania (A5)
const int WARNING_BEEP = 1760;  // Dźwięk ostrzegawczy (A6)
const int ENGINE_START = 440;   // Podstawowy ton silnika (A4)

void setup()
{
  // Inicjalizacja paska LED
  strip.begin();
  strip.show();
  strip.setBrightness(150); // Zwiększamy jasność do 150/255

  // Inicjalizacja buzzera
  pinMode(BUZZER_PIN, OUTPUT);
}

// Funkcja generująca losowy kolor płomienia
uint32_t getRandomFlameColor()
{
  return FLAME_COLORS[random(NUM_COLORS)];
}

// Funkcja pomocnicza do przyciemniania koloru (przenosimy przed chaseEffect)
uint32_t dimColor(uint32_t color, float factor)
{
  uint8_t r = ((color >> 16) & 0xFF) * factor;
  uint8_t g = ((color >> 8) & 0xFF) * factor;
  uint8_t b = (color & 0xFF) * factor;
  return strip.Color(r, g, b);
}

// Funkcja symulująca efekt płomienia
void updateFlameEffect(int intensity)
{
  // Najpierw ustawiamy wszystkie LEDy
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Odwracamy indeks, żeby efekt szedł od dołu (LED_PIN jest na dole rakiety)
    int ledIndex = NUM_LEDS - 1 - i;

    // Intensywność jest większa dla dolnych LEDów
    int adjustedIntensity = intensity * (1.0 - (float)i / NUM_LEDS * 0.3);

    // Zawsze zapalamy dolne LEDy (teraz to będą te z większymi indeksami)
    if (i < 3 || random(100) < adjustedIntensity)
    {
      uint32_t color = getRandomFlameColor();

      // Górne LEDy są przyciemnione (teraz to te z mniejszymi indeksami)
      if (i > NUM_LEDS - 4)
      {
        uint8_t r = (uint8_t)((color >> 16) & 0xFF) * 0.5;
        uint8_t g = (uint8_t)((color >> 8) & 0xFF) * 0.5;
        uint8_t b = (uint8_t)(color & 0xFF) * 0.5;
        color = strip.Color(r, g, b);
      }

      strip.setPixelColor(ledIndex, color);
    }
    else
    {
      strip.setPixelColor(ledIndex, 0); // Wyłączony LED
    }
  }
  strip.show();
  delay(50); // Dodajemy małe opóźnienie dla lepszego efektu migotania
}

// Funkcja odtwarzająca dźwięk startu
void playStartSound(int duration, int frequency)
{
  tone(BUZZER_PIN, frequency, duration);
}

// Nowa funkcja do efektu pulsowania
void pulseEffect(uint32_t color, int duration)
{
  for (int brightness = 0; brightness <= 255; brightness += 15)
  {
    strip.fill(color);
    strip.setBrightness(brightness);
    strip.show();
    delay(duration / 32);
  }
  for (int brightness = 255; brightness >= 0; brightness -= 15)
  {
    strip.fill(color);
    strip.setBrightness(brightness);
    strip.show();
    delay(duration / 32);
  }
  strip.setBrightness(150); // Przywracamy domyślną jasność
}

// Nowa funkcja do efektu przesuwania się światła
void chaseEffect(uint32_t color, int duration)
{
  // Definiujemy kolory dla efektu
  uint32_t yellow = strip.Color(255, 255, 0); // Żółty
  uint32_t orange = strip.Color(255, 165, 0); // Pomarańczowy
  uint32_t red = strip.Color(255, 0, 0);      // Czerwony

  for (int i = 0; i < NUM_LEDS; i++)
  {
    strip.clear();

    // Główny pixel w kolorze żółtym
    strip.setPixelColor(i, yellow);

    if (i > 0)
      strip.setPixelColor(i - 1, orange); // Pierwszy ślad pomarańczowy

    if (i > 1)
      strip.setPixelColor(i - 2, dimColor(red, 0.3)); // Drugi ślad czerwony, przyciemniony

    strip.show();
    delay(duration / NUM_LEDS);
  }
}

void loop()
{
  // Efekt przygotowania do startu
  tone(BUZZER_PIN, WARNING_BEEP, 500);
  for (int i = 0; i < 3; i++)
  {
    chaseEffect(strip.Color(255, 255, 0), 500);
  }
  noTone(BUZZER_PIN);

  // Sekwencja startowa - odliczanie NASA style (T-10 do T-0)
  for (int countdown = 10; countdown >= 0; countdown--)
  {
    // Sygnał dźwiękowy odliczania
    if (countdown > 0)
    {
      tone(BUZZER_PIN, COUNTDOWN_BEEP, 100);
    }
    else
    {
      // Specjalny sygnał dla T-0
      tone(BUZZER_PIN, WARNING_BEEP, 200);
    }

    // Efekt świetlny dla każdej sekundy
    if (countdown > 0)
    {
      pulseEffect(strip.Color(255, 0, 0), 900); // Czerwony puls
    }
    else
    {
      pulseEffect(strip.Color(255, 255, 255), 100); // Biały puls
      pulseEffect(strip.Color(255, 0, 0), 100);     // Czerwony puls
    }

    delay(100);
  }

  // Sekwencja startowa - narastanie mocy
  for (int intensity = 0; intensity <= 100; intensity += 5)
  {
    // Zwiększamy częstotliwość dźwięku wraz z intensywnością
    int frequency = ENGINE_START + (intensity * 10);
    tone(BUZZER_PIN, frequency);
    updateFlameEffect(intensity);
    delay(50);
  }

  // Faza głównego ciągu
  for (int i = 0; i < 40; i++)
  {
    updateFlameEffect(100);

    // Efekt turbulencji
    if (i % 5 == 0)
    {
      strip.setBrightness(200);
      // Dodajemy lekką modulację dźwięku
      tone(BUZZER_PIN, ENGINE_START + 1000 + random(-100, 100));
      delay(50);
      strip.setBrightness(150);
    }
    else
    {
      tone(BUZZER_PIN, ENGINE_START + 1000);
    }

    delay(100);
  }

  // Faza wygaszania
  for (int intensity = 100; intensity >= 0; intensity -= 2)
  {
    // Zmniejszamy częstotliwość dźwięku wraz z intensywnością
    int frequency = ENGINE_START + (intensity * 10);
    tone(BUZZER_PIN, frequency);
    updateFlameEffect(intensity);
    delay(50);
  }
  noTone(BUZZER_PIN);

  // Efekt końcowy - delikatne migotanie
  for (int i = 0; i < 3; i++)
  {
    tone(BUZZER_PIN, ENGINE_START / 2, 200);
    pulseEffect(strip.Color(255, 69, 0), 500);
    delay(300);
  }

  // Wyciszamy buzzer na wszelki wypadek
  noTone(BUZZER_PIN);

  // Pauza przed kolejnym startem
  strip.clear();
  strip.show();
  delay(3000);
}
