//By Matthew Masi & Michael Gravino
#include <fstream>
#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <thread>

//pressure values in psi, acceptable range between 10 and 15
int presVals[30] = {14, 15, 14, 13, 14, 13 ,12 ,11, 12, 10,
                    9, 9, 8, 10, 12, 13, 14, 14, 14, 16,
                    16, 16, 12, 13, 14, 15, 14, 13, 14, 12};

//temperature values in celcius, acceptable range between 15 and 25
int tempVals[30] = {21, 21, 20, 19, 18, 17 ,14, 13, 12, 17,
                    18, 19, 20, 21, 22, 23, 24, 24, 25, 26,
                    27, 25, 24, 23, 22, 21, 20, 21, 21, 21};

//fuel tank values in kL, max tank size = 200 000L
int fuelVals[30] = {180, 180, 175, 170, 160, 150 ,150, 140, 130, 120,
                    115, 110, 105, 100, 95, 85, 70, 65, 60, 55,
                    50, 45, 40, 35, 30, 25, 15, 10, 10, 5};

int presCtr = 0;
int tempCtr = 0;
int showDisplay[3] = {0, 0, 0};
double presCh = 0;
double tempCh = 0;
int fuelCon = 0;
bool warning = false;
std::string warningType[3] = {"fuel", "pres", "temp"};
int warnArray[3] = {0, 0, 0};
int flag1, flag2, flag3 = 0;
bool smoke = false;
bool smokeClear = false;
bool clearScreen = false;
bool enterSmoke = false;
bool p, t, f, c = false;

//timer class used for 1 second polling and to timestamp smoke detection
class Timer {
private:
    bool resetted;
    bool running;
    unsigned long beg;
    unsigned long end;

public:
Timer() {
	resetted = true;
	running = false;
	beg = 0;
	end = 0;
}

//start timer
void start() {
	if(! running) {
		if(resetted)
			beg = (unsigned long) clock();
		else
			beg -= end - (unsigned long) clock();
		running = true;
		resetted = false;
	}
}

//stop timer
void stop() {
	if(running) {
		end = (unsigned long) clock();
		running = false;
	}
}

//reset timer
void reset() {
	bool wereRunning = running;
	if(wereRunning)
		stop();
	resetted = true;
	beg = 0;
	end = 0;
	if(wereRunning)
		start();
}

bool isRunning() {
	return running;
}

//get time in seconds
unsigned long getTimeSec() {
	if(running)
		return ((unsigned long) clock() - beg) / CLOCKS_PER_SEC;
	else
		return (end - beg) / CLOCKS_PER_SEC;
}

//get timer in miliseconds
unsigned long getTime() {
	if(running)
		return ((unsigned long) clock() - beg);
	else
		return end - beg;
}
};

Timer timer; //instantiate global timer object

//class for tasks that includes execution time, deadline, and period
class Task
{
private:
    std::string taskName;
    int execTime;
    int period;
    int deadline;

public:

void setName(std::string name){
    taskName = name;
}

void setExecTime(int time){
    execTime = time;
}

void setPeriod(int time){
    period = time;
}

void setDeadline(int time){
    deadline = time;
}

std::string getName(){
    return taskName;
}

int getExecTime(){
    return execTime;
}

int getPeriod(){
    return period;
}

int getDeadline(){
    return deadline;
}
};

Task *schedule = new Task[4];   //instantiate array of task objects

//console class used to parse commands and display info on the monitor
class Console
{
private:

public:

//show keyboard options
void displayOptions(){
    std::cout <<std::endl <<"*****Keyboard and Monitor*****" << std::endl;
    std::cout << "p -> Pressure rate of change." << std::endl << "t -> Temperature rate of change."
            << std::endl << "f -> Fuel consumption." <<std::endl << "c -> Clear monitor" << std::endl
            << "1 -> Trigger smoke 1" <<std::endl << "2 -> Trigger smoke 2" << std::endl
            << "a -> Acknowledge Warnings" <<std::endl;
}

//parse the different keyboard commands
void parseCommands(){
    displayOptions();
    int flag = 0;
    std::string input;
    std::string buff;

    while(1){
        if(flag == 0)
            std::cin >> input;

        else{
            input = buff;
            buff.clear();
        }

        if(warning == true || smokeClear == true){
            if(input != "a" && input != "1" && input != "2"){
                buff = input;
                flag = 0;
            }

            else if(input == "a"){
                flag = 1;
                for(int i=0; i<3; i++){
                    if(warnArray[i] == 1){
                        warnArray[i] = 0;
                    }
                }
                if(presCtr >= 3)
                    presCtr = 0;

                if(tempCtr >= 3)
                    tempCtr = 0;

                clearScreen = true;
                if(smokeClear == false && smoke == true){
                    smokeClear = true;
                    smokeStopped();
                    writeFile("cleared", timer.getTime()/1000.0);
                }
                else
                    smokeClear = false;
                smoke = false;
                warning = false;
            }
        }

        else if(warning == false)
        {
            flag = 0;
            if(input == "p"){
                showDisplay[0] = 1;
                p = true;
            }
            if(input == "t"){
                showDisplay[1] = 1;
                t = true;
            }
            if(input == "f"){
                showDisplay[2] = 1;
                f = true;
            }
            if(input == "c"){
                showDisplay[0] = 0;
                showDisplay[1] = 0;
                showDisplay[2] = 0;
                c = true;
            }
            if(input == "1"){
                smoke = true;
                warning = true;
                enterSmoke = true;
                writeFile("detected", timer.getTime()/1000.0);
            }
            if(input == "2"){
                smoke = true;
                warning = true;
                enterSmoke = true;
                writeFile("detected", timer.getTime()/1000.0);
            }
        }
    }
}

void presChange(){
    std::cout << "Pressure rate of change: " << presCh <<std::endl;
}

void tempChange(){
    std::cout << "Temperature rate of change: " << tempCh <<std::endl;
}

void fuelConsumption(){
    std::cout << "Fuel consumption: " << fuelCon <<std::endl;
}

void displayWarning(bool warn){
    if(warnArray[1] == 1 && warn == true)
        std::cout << "!!!!!!!!Warning Pressure Not Stable Press a + enter to Ack!!!!!!!!" <<std::endl;

    if(warnArray[2] == 1 && warn == true)
        std::cout << "!!!!!!!!Warning Temperature Not Stable Press a + enter to Ack!!!!!!!!" <<std::endl;

    if(warnArray[0] == 1 && warn == true)
        std::cout << "!!!!!!!!Warning Fuel Low Press a + enter to Ack!!!!!!!!" <<std::endl;
}

void smokeDetection(){
    std::cout << "!!!!!!!!Smoke Detected Press a + enter to Ack!!!!!!!!" <<std::endl;
}

void smokeStopped(){
    std::cout << "!!!!!!!!Smoke Cleared Press a + enter to Ack!!!!!!!!" <<std::endl;
}

//write smoke detection and clearance on magnetic tape
void writeFile (std::string onOff, double time)
{
    std::ofstream myfile("magneticTape.txt", std::ios_base::app | std::ios_base::out);
    myfile << "Smoke " << onOff <<" Time: " << time <<std::endl;
    myfile.close();
}
};

//class used to display dials and lamps
class Display
{
private:

public:

void showDials(std::string sensorName , double sensorVal, std::string unit)
{
    std::cout << sensorName << " " << sensorVal << unit << std::endl;
}

void showLamps(std::string lampName , std::string lampColour)
{
    std::cout << lampName << " " << lampColour << std::endl;
}
};

//schedule periodic tasks
class Scheduler
{
private:
    int schedIndex = 0;

public:
void addTask(std::string taskName, int execTime, int period, int deadline){
    schedule[schedIndex].setName(taskName);
    schedule[schedIndex].setExecTime(execTime);
    schedule[schedIndex].setPeriod(period);
    schedule[schedIndex].setDeadline(deadline);
    schedIndex++;
}
};

//system class used to put everything together and also poll dials
class System
{
private:
    Console console;
    Display display;
    Scheduler scheduler;
    bool _alive;
    bool _active;

public:

void addDials(std::string sensorName , double sensorVal, std::string unit){
    display.showDials(sensorName, sensorVal, unit);
}

void addLamps(std::string lampName , std::string lampColour){
    display.showLamps(lampName, lampColour);
}

void addTask(std::string taskName, int execTime, int period, int deadline){
    scheduler.addTask(taskName, execTime, period, deadline);
}

//function that polls sensors
void pollingThread(){
    int prev = 0;
    int next = 1;
    int pres, temp, fuel = 0;
    std::string lamp = "green";

    while(1)
    {
        next = timer.getTimeSec();
        if(next != prev || (warnArray[0] == 1 && flag1 <=1) || (warnArray[1] == 1 && flag2 <=1)
           || (warnArray[2] == 1 && flag3 <=1) || clearScreen == true ||enterSmoke == true
           || p == true || t == true || f == true || c == true)
        {
            clearScreen = false;
            enterSmoke = false;
            p = false;
            t = false;
            f = false;
            c = false;
            prev = next;
            fuel = fuelVals[timer.getTimeSec() % 30];
            fuelCon = 200 - fuel;

            pres = presVals[timer.getTimeSec() % 30];
            presCh += pres;
            presCh /= timer.getTimeSec();

            temp = tempVals[timer.getTimeSec() % 30];
            tempCh += temp;
            tempCh /= timer.getTimeSec();

            system ("CLS");
            std::cout << "*****Dials*****" << std::endl;
            display.showDials("Fuel Dial:", fuel, " kL");
            display.showDials("Pres Dial:", pres, " psi");
            display.showDials("Temp Dial:", temp, " c");

            std::cout <<std::endl <<"*****Lamps*****" << std::endl;
            lamp = checkFuel(fuel);
            display.showLamps("Fuel Lamp:", lamp);

            lamp = checkPresTemp("Pres", pres);
            display.showLamps("Pres Lamp:", lamp);

            lamp = checkPresTemp("Temp", temp);
            display.showLamps("Temp Lamp:", lamp);

            if(smoke == false)
                display.showLamps("Smoke Lamp:", "green");
            else
                display.showLamps("Smoke Lamp:", "red");

            console.displayOptions();
            if(showDisplay[0] == 1)
                console.presChange();
            if(showDisplay[1] == 1)
                console.tempChange();
            if(showDisplay[2] == 1)
                console.fuelConsumption();

            if(smoke == true)
                console.smokeDetection();
            if(smoke == false && smokeClear == true)
                console.smokeStopped();
            console.displayWarning(warning);
        }
    }
}

//check to make sure fuel is above 20%
std::string checkFuel(int reading){
    std::string lamp = "green";
    double percentage;
    percentage = (reading/200.0)*100;

    if(percentage <= 10.0){
        lamp = "red";
        warning = true;
        warnArray[0] = 1;
        flag1++;
    }
    return lamp;
}

//make sure pressure and temperature is withing working range
std::string checkPresTemp(std::string name, int reading){
    std::string lamp = "green";
    //check pressure
    if (name == "Pres"){
        if(reading < 10 || reading > 15 )
            presCtr++;

        else
            presCtr = 0;

        if(presCtr >= 3){
            lamp = "red";
            warning = true;
            warnArray[1] = 1;
            flag2++;
        }
    }

    //check temperature
    else if(name == "Temp"){
        if(reading < 15 || reading > 25 )
            tempCtr++;

        else
            tempCtr =0;

        if(tempCtr >= 3){
            lamp = "red";
            warning = true;
            warnArray[2] = 1;
            flag3++;
        }
    }
    return lamp;
}

//start system
void execute(){
    timer.start();  //start timer
    std::thread t1(System::pollingThread, System());    //thread that controls polling
    std::thread t2(console.parseCommands, Console());   //thread that controls console
    t1.join();
    t2.join();
}
};

int main()
{
    System system;

    //add dials
    std::cout <<"*****Dials*****" << std::endl;
    system.addDials("Fuel Dial:", 0, " kL");
    system.addDials("Pres Dial:", 0, " psi");
    system.addDials("Temp Dial:", 0, " c");

    //add lamps
    std::cout <<std::endl <<"*****Lamps*****" << std::endl;
    system.addLamps("Fuel Lamp:", "green");
    system.addLamps("Pres Lamp:", "green");
    system.addLamps("Temp Lamp:", "green");
    system.addLamps("Smoke Lamp:", "green");

    //add periodic tasks
    system.addTask("Fuel",  10, 1000, 1000);
    system.addTask("Pres",  10, 1000, 1000);
    system.addTask("Temp",  10, 1000, 1000);

    //run system
    system.execute();
}
