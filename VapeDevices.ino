/*

Pin A6, A7 - analog only

Нажатие на кнопку включает/выключает двигатель, подавая сигнал LOW/HIGH на вход ENABLE драйвера A4988.
С помощью переключателя выбираем направление вращения двигателя (сигнал с переключателя подается напрямую на вход DIR драйвера A4988).
C помощью потенциометра мы выбираем один из режимов микрошага, тем самым устанавливая скорость вращения.


A4988 pins:
	ENABLE – включение/выключение драйвера
	MS1, MS2, MS3 – контакты для установки микрошага
	RESET - cброс микросхемы
	STEP - генерация импульсов для движения двигателей (каждый импульс – шаг), можно регулировать скорость двигателя
	SLEEP — вывод включения спящего режима, если подтянуть его к низкому состоянию драйвер перейдет в спящий режим.
	DIR – установка направление вращения
	VMOT – питание для двигателя (8 – 35 В) (обязательное наличие конденсатора на 100 мкФ ).
	GND – общий
	2B, 2A, 1A, 1B – для подключения обмоток двигателя
	VDD – питание микросхемы (3.5 – 5В)
*/

char *Version = "VapeDevices 1.0 by LuXiMaL";

#define PIN_INPUT_POT A7 // Пин для подключения потенциометра

#define PIN_BUTTON_START 9	// Кнопка СТАРТ
#define PIN_BUTTON_PLUS 10	// Кнопка "+" кол-во оборотов
#define PIN_BUTTON_MINUS 11 // Кнопка "-" кол-во оборотов
#define PIN_BUTTON_L_R 2	// Пин для подключения переключателя управления направлением вращения двигателя Left/Right

#define PIN_OUTPUT_DIR 3   // Пин для подключения к контакту DIR А4988, управляется кнопкой на PIN_BUTTON_L_R
#define PIN_OUTPUT_STEP 4  // Пин для подключения к контакту STEP А4988
#define PIN_OUTPUT_START 8 // Пин для подключения к контакту ENABLE А4988, управляется кнопкой на PIN_BUTTON_START

int Turns = 3;		  // Число оборотов (3)
int Steps = 200;	  // Количество шагов на 1 оборот, зависит от двигателя (200)
int Speed = 0;		  // Скорость двигателя (0), использование не желательно)))
int r_Dir = 2;		  // На какую часть от Steps сделать вращение в обратную сторону (2)
int Flash = 2;		  // Количество морганий при индикации изменения скорости, при вращении потенциометра (2)
int Type_7_Digit = 0; // Тип 7-Digit индикатора. 0 - Общий Анод, 1 - Общий катод

int pins_7_Digit[7] = {A0, A1, A2, A3, A4, A5, 12};																				   // список выводов Arduino для подключения к разрядам a-g семисегментного индикатора
byte numbers[10] = {B11111100, B01100000, B11011010, B11110010, B01100110, B10110110, B10111110, B11100000, B11111110, B11110110}; // массив значений для вывода цифр 0-9

// массив пинов для MS1,MS2,MS3 Микрошаг
int pins_micro_step[] = {7, 6, 5};
int micro_step[5][3] = {
	{1, 1, 1}, // 1/16
	{1, 1, 0}, // 1/8
	{0, 1, 0}, // 1/4
	{1, 0, 0}, // 1/2
	{0, 0, 0}  // 1
};

int micro_step_ratio[] = {16, 8, 4, 2, 1};

int prev_Button = 0;
int cur_Button = 0;
int prev_Button_PLUS = 0;
int cur_Button_PLUS = 0;
int prev_Button_MINUS = 0;
int cur_Button_MINUS = 0;
int prev_Button_START = 0;
int cur_Button_START = 0;

boolean Start = false;

int mode_previos;

void setup()
{
	//	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(115200);
	Serial.println(Version);

	pinMode(PIN_OUTPUT_DIR, OUTPUT);
	pinMode(PIN_OUTPUT_STEP, OUTPUT);
	pinMode(PIN_OUTPUT_START, OUTPUT);

	for (int i = 0; i < 3; i++)
	{//	Сконфигурировать контакты как выходы для MS1,MS2,MS3
		pinMode(pins_micro_step[i], OUTPUT);
	}

	for (int i = 0; i < 7; i++)
	{// Сконфигурировать контакты как выходы для семисегментного индикатора
		pinMode(pins_7_Digit[i], OUTPUT);
	}

	//	начальные значения
	digitalWrite(PIN_OUTPUT_DIR, 1);
	digitalWrite(PIN_OUTPUT_STEP, 0);
	digitalWrite(PIN_OUTPUT_START, 1); // не разрешать
}

void loop()
{

	//	Проверка нажатия кнопки PIN_BUTTON_PLUS
	cur_Button_PLUS = debounce(prev_Button_PLUS, PIN_BUTTON_PLUS);
	if (prev_Button_PLUS == 0 && cur_Button_PLUS == 1)
	{
		Turns = Turns + 1;
		if (Turns > 9)
		{
			Turns = 9;
		}
		// Serial.println(String("PLUS: ") + Turns);
	}
	prev_Button_PLUS = cur_Button_PLUS;

	//	Проверка нажатия кнопки PIN_BUTTON_MINUS
	cur_Button_MINUS = debounce(prev_Button_MINUS, PIN_BUTTON_MINUS);
	if (prev_Button_MINUS == 0 && cur_Button_MINUS == 1)
	{
		Turns = Turns - 1;
		if (Turns < 1)
		{
			Turns = 1;
		}
		// Serial.println(String("MINUS: ") + Turns);
	}
	prev_Button_MINUS = cur_Button_MINUS;

	//	Вывести на индикатор текущее значение кол-ва оборотов
	showNumber(Turns);
	//	Turns = (Turns + 1) % 10;	// Перевернем байты

	//	Получить режим микрошага 1-5 с потенциометра
	int mode = map(analogRead(PIN_INPUT_POT), 0, 1024, 0, 5);
	// установить режим микро шага
	for (int i = 0; i < 3; i++)
	{
		digitalWrite(pins_micro_step[i], micro_step[mode][i]);
	}

	//	Отображение показаний микрошага при изменении
	int mode_current = mode;
	if (mode_previos != mode_current)
	{
		mode_previos = mode_current;
		for (int i = 0; i < Flash; i++)
		{
			showNumber(11);
			delay(500);
			showNumber(mode);
			delay(500);
		}
	}

	//	Направление вращения
	boolean direction = digitalRead(PIN_BUTTON_L_R);
	digitalWrite(PIN_OUTPUT_DIR, direction);

	//	Проверка нажатия кнопки PIN_BUTTON_START
	cur_Button_START = debounce(prev_Button_START, PIN_BUTTON_START);
	if (prev_Button_START == 0 && cur_Button_START == 1)
	{
		Start = !Start;
		digitalWrite(PIN_OUTPUT_START, !Start);
	}
	prev_Button_START = cur_Button_START;

	//	Если нажата кнопка, сделать Turns оборотов в направлении direction и часть оборота обратно
	if (Start == true)
	{

		int Steps_ = Steps * micro_step_ratio[mode];

		Serial.println(String("Микрошаг:") + mode);
		Serial.println(String("Множитель:") + micro_step_ratio[mode]);
		Serial.println(String("Обороты:") + Turns);
		Serial.println(String("Шаги:") + Steps_);
		Serial.println(String("Направление:") + direction);

		Serial.println("ВПЕРЕД");
		for (int j = 0; j < Steps_ * Turns; j++)
		{
			Serial.println(String("Вперед: ") + j);
			digitalWrite(PIN_OUTPUT_STEP, HIGH);
			delay(Speed);
			digitalWrite(PIN_OUTPUT_STEP, LOW);
			delay(Speed);
		}
		Serial.println("НАЗАД");
		digitalWrite(PIN_OUTPUT_DIR, !direction);
		for (int j = 0; j < round(Steps_ / r_Dir); j++)
		{
			Serial.println(String("Назад: ") + j);
			digitalWrite(PIN_OUTPUT_STEP, HIGH);
			delay(Speed);
			digitalWrite(PIN_OUTPUT_STEP, LOW);
			delay(Speed);
		}
		digitalWrite(PIN_OUTPUT_DIR, direction);
	}
	Start = false;
}

// Проверка на дребезг
int debounce(int previos, int pin)
{
	int current = digitalRead(pin);
	if (previos != current)
	{
		delay(5);
		current = digitalRead(pin);
		return current;
	}
}

// Вывод цифры на семисегментный индикатор
void showNumber(int num)
{
	for (int i = 0; i < 7; i++)
	{
		if (bitRead(numbers[num], 7 - i) == Type_7_Digit) // зажечь сегмент 0 - Общий Анод, 1 - Общий катод
			digitalWrite(pins_7_Digit[i], 1);
		else // потушить сегмент
			digitalWrite(pins_7_Digit[i], 0);
	}
}
