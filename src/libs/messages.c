#include "messages.h"
#include "devices.h"
#include <stdio.h>

//Trasforma lo stato in una stringa
void stateToStr(char* string, const int state) {
    switch(state) {
        case 0: strcpy(string, "off"); break;
        case 1: strcpy(string, "on"); break;
        case 2: strcpy(string, "off (override)"); break;
        case 3: strcpy(string, "on (override)"); break;
        default: strcpy(string, "invalid state"); break;
    }
}

//Trasforma time_t in una stringa
void timeToStr(char* string, const time_t _time) {
    if(_time) {
        //Converte _time da time_t in una stringa
        struct tm * struct_tm = localtime( &_time );
        strftime(string, DATESIZE, "%F - %T", struct_tm);
    }else{
        //Il valore di default 0 è tradotto in 'null'
        strcpy(string, "null");
    }
}

// CHECK
MessageCheck createCheck(const int id, const int idAncestor) {
    MessageCheck msg = {0, id, idAncestor, -1, -1 , -1, -1};
    return msg;
}

void serializeCheck(char* string, const MessageCheck msg) {
    sprintf(string, "check %d %d %d %d %d %d %d",
        msg.ack,
        msg.id,
        msg.idAncestor,
        msg.type,
        msg.pid,
        msg.isControl,
        msg.oldestParent
    );
}

MessageCheck deSerializeCheck(const char* string) {
    MessageCheck msg;
    sscanf(string, "check %d %d %d %d %d %d %d",
        &msg.ack,
        &msg.id,
        &msg.idAncestor,
        &msg.type,
        &msg.pid,
        &msg.isControl,
        &msg.oldestParent
    );
    return msg;
}

void printCheck(const MessageCheck msg) {
    printf("Msg check:\n- ack: %d\n- id: %d\n- idAncestor: %d\n- type: %d\n- pid: %d\n -isControl: %d\n -oldestParent: %d\n",
        msg.ack,
        msg.id,
        msg.idAncestor,
        msg.type,
        msg.pid,
        msg.isControl,
        msg.oldestParent
    );
}

// DEL

MessageDel createDel(int id, int save) {
    MessageDel msg = {0, id, 1, save};
    return msg;
}

void serializeDel(char* string, const MessageDel msg) {
    sprintf(string, "del %d %d %d %d",
        msg.ack,
        msg.id,
        msg.updateParent,
        msg.save
    );
}

MessageDel deSerializeDel(const char* string) {
    MessageDel msg;
    sscanf(string, "del %d %d %d %d",
        &msg.ack,
        &msg.id,
        &msg.updateParent,
        &msg.save
    );
    return msg;
}

void printDel(const MessageDel msg) {
    printf("Msg del:\n- ack: %d\n- id: %d\n- updateParent: %d\n- save: %d\n",
        msg.ack,
        msg.id,
        msg.updateParent,
        msg.save
    );
}

// INFO

MessageInfo createInfo(const int id) {
    MessageInfo msg = {0, id, -1, "null"};
    return msg;
}

void serializeInfo(char* string, const MessageInfo msg) {
    sprintf(string, "info %d %d %d %s\n",
        msg.ack,
        msg.id,
        msg.type,
        msg.device
    );
}

MessageInfo deSerializeInfo(const char* string) {
    MessageInfo msg;
    sscanf(string, "info %d %d %d %[^\n]",
        &msg.ack,
        &msg.id,
        &msg.type,
        msg.device
    );
    return msg;
}

void printInfo(const MessageInfo msg) {
    printf("Msg info:\n- ack: %d\n- id: %d\n- type: %d\n- device: %s\n",
        msg.ack,
        msg.id,
        msg.type,
        msg.device
    );
}

// SWITCH

MessageSwitch createSwitch(const int id, const char* label, const char* pos) {
    MessageSwitch msg = {0, id, "", "", -1};
    strcpy(msg.label, label);
    strcpy(msg.pos, pos);
    return msg;
}

void serializeSwitch(char* string, const MessageSwitch msg) {
    sprintf(string, "switch %d %d %s %s %d",
        msg.ack,
        msg.id,
        msg.label,
        msg.pos,
        msg.newState
    );
}

MessageSwitch deSerializeSwitch(const char* string) {
    MessageSwitch msg;
    sscanf(string, "switch %d %d %s %s %d",
        &msg.ack,
        &msg.id,
        msg.label,
        msg.pos,
        &msg.newState
    );
    return msg;
}

void printSwitch(const MessageSwitch msg) {
    printf("Msg switch:\n- ack: %d\n- id: %d\n- label: %s\n- pos: %s\n- newState: %d\n",
        msg.ack,
        msg.id,
        msg.label,
        msg.pos,
        msg.newState
    );
}

// LIST

MessageList createList() {
    MessageList msg = {0, 1};
    return msg;
}

void serializeList(char* string, const MessageList msg) {
    sprintf(string, "list %d %d",
        msg.ack,
        msg.level
    );
}

MessageList deSerializeList(const char* string) {
    MessageList msg;
    sscanf(string, "list %d %d",
        &msg.ack,
        &msg.level
    );
    return msg;
}

void printList(const MessageList msg) {
    printf("Msg list:\n- ack: %d\n- level: %d\n",
        msg.ack,
        msg.level
    );
}

//LINK

MessageLink createLink(int id1, int id2, int type_id1) {
    MessageLink msg = {0, id1, id2, type_id1};
    return msg;
}

void serializeLink(char* string, const MessageLink msg) {
    sprintf(string, "link %d %d %d %d",
        msg.ack,
        msg.id1,
        msg.id2,
        msg.type_id1
    );
}

MessageLink deSerializeLink(const char* string) {
    MessageLink msg;
    sscanf(string, "link %d %d %d %d",
        &msg.ack,
        &msg.id1,
        &msg.id2,
        &msg.type_id1
    );
    return msg;
}

void printLink(const MessageLink msg) {
    printf("Msg link:\n- ack: %d\n- id1: %d\n- id2: %d\n- type_id1: %d\n",
        msg.ack,
        msg.id1,
        msg.id2,
        msg.type_id1
    );
}

// UPDATE

MessageUpdate createUpdate(int ack, int type) {
    MessageUpdate msg = {ack, type};
    return msg;
}

void serializeUpdate(char* string, const MessageUpdate msg) {
    sprintf(string, "update %d %d",
        msg.ack,
        msg.type
    );
}

MessageUpdate deSerializeUpdate(const char* string) {
    MessageUpdate msg;
    sscanf(string, "update %d %d",
        &msg.ack,
        &msg.type
    );
    return msg;
}

void printUpdate(const MessageUpdate msg) {
    printf("Msg update:\n- ack: %d\n- type: %d\n",
        msg.ack,
        msg.type
    );
}


// CONTROLLER

void printController(const Controller cont) {
    printf("\nController_01:\n- id: 1\n- state: %s\n- switch: %s\n- num: %d\n\n",
        cont.state ? "on" : "off",
        cont.mainSwitch ? "on" : "off",
        getDimList(cont.list)
    );
}

// HUB

void serializeHub(char* string, const Hub hub) {
    sprintf(string, "hub %d %s %d %d",
        hub.id,
        hub.name,
        hub.state,
        hub.type
    );
}

Hub deSerializeHub(const char* string) {
    Hub hub;
    sscanf(string, "hub %d %s %d %d",
        &hub.id,
        hub.name,
        &hub.state,
        &hub.type
    );
    return hub;
}

void printHub(const Hub hub) {
    printf("\n%s:\n- id: %d\n\n",
        hub.name,
        hub.id
    );
}

// TIMER

void serializeTimer(char* string, const Timer timer) {
    sprintf(string, "timer %d %s %d %ld %ld",
        timer.id,
        timer.name,
        timer.state,
        timer.begin,
        timer.end
    );
}

Timer deSerializeTimer(const char* string) {
    Timer timer;
    sscanf(string, "timer %d %s %d %ld %ld",
        &timer.id,
        timer.name,
        &timer.state,
        &timer.begin,
        &timer.end
    );
    return timer;
}

void printTimer(const Timer timer) {
    char strbegin[DATESIZE];
    char strend[DATESIZE];
    timeToStr(strbegin, timer.begin);
    timeToStr(strend, timer.end);

    char strstate[16];
    stateToStr(strstate, timer.state);

    printf("\n%s:\n- id: %d\n- begin: %s\n- end: %s\n\n",
        timer.name,
        timer.id,
        strbegin,
        strend
    );
}

// BULB

void serializeBulb(char* string, const Bulb bulb) {
    sprintf(string, "bulb %d %s %d %d %ld",
        bulb.id,
        bulb.name,
        bulb.state,
        bulb.mainSwitch,
        bulb.activeTime
    );
}

Bulb deSerializeBulb(const char* string) {
    Bulb bulb;
    sscanf(string, "bulb %d %s %d %d %ld",
        &bulb.id,
        bulb.name,
        &bulb.state,
        &bulb.mainSwitch,
        &bulb.activeTime
    );
    return bulb;
}

void printBulb(const Bulb bulb) {
    char strstate[16];
    stateToStr(strstate, bulb.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- mainSwitch: %s\n- activeTime: %ld seconds\n\n",
        bulb.name,
        bulb.id,
        strstate,
        bulb.mainSwitch ? "on" : "off",
        bulb.activeTime
    );
}

// WINDOW

void serializeWindow(char* string, const Window window) {
    sprintf(string, "window %d %s %d %d %d %ld",
        window.id,
        window.name,
        window.state,
        window.openSwitch,
        window.closeSwitch,
        window.openTime
    );
}

Window deSerializeWindow(const char* string) {
    Window window;
    sscanf(string, "window %d %s %d %d %d %ld",
        &window.id,
        window.name,
        &window.state,
        &window.openSwitch,
        &window.closeSwitch,
        &window.openTime
    );
    return window;
}

void printWindow(const Window window) {
    char strstate[16];
    stateToStr(strstate, window.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- openSwitch: %s\n- closeSwitch: %s\n- openTime: %ld seconds\n\n",
        window.name,
        window.id,
        strstate,
        window.openSwitch ? "on" : "off",
        window.closeSwitch ? "on" : "off",
        window.openTime
    );
}

// FRIDGE

void serializeFridge(char* string, const Fridge fridge) {
    sprintf(string, "fridge %d %s %d %d %d %ld %ld %d %d",
        fridge.id,
        fridge.name,
        fridge.state,
        fridge.mainSwitch,
        fridge.thermostat,
        fridge.openTime,
        fridge.delay,
        fridge.perc,
        fridge.temp
    );
}

Fridge deSerializeFridge(const char* string) {
    Fridge fridge;
    sscanf(string, "fridge %d %s %d %d %d %ld %ld %d %d",
        &fridge.id,
        fridge.name,
        &fridge.state,
        &fridge.mainSwitch,
        &fridge.thermostat,
        &fridge.openTime,
        &fridge.delay,
        &fridge.perc,
        &fridge.temp
    );
    return fridge;
}

void printFridge(const Fridge fridge) {
    char strstate[16];
    stateToStr(strstate, fridge.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- mainSwitch: %s\n- thermostat: %d °C\n- openTime: %ld seconds\n- delay: %ld seconds\n- perc: %d%%\n- temp: %d °C\n\n",
        fridge.name,
        fridge.id,
        strstate,
        fridge.mainSwitch ? "on" : "off",
        fridge.thermostat,
        fridge.openTime,
        fridge.delay,
        fridge.perc,
        fridge.temp
    );
}

// TIMER OF BULB

TimerOfBulb createTimerOfBulb(const Timer timer, const Bulb bulb) {
    TimerOfBulb tbulb = {
        timer.id,
        "",
        bulb.state,
        bulb.mainSwitch,
        bulb.activeTime,
        timer.begin,
        timer.end
    };
    strcpy(tbulb.name, timer.name);
    return tbulb;
}

void serializeTimerOfBulb(char* string, const TimerOfBulb tbulb) {
    sprintf(string, "tbulb %d %s %d %d %ld %ld %ld",
        tbulb.id,
        tbulb.name,
        tbulb.state,
        tbulb.mainSwitch,
        tbulb.activeTime,
        tbulb.begin,
        tbulb.end
    );
}

TimerOfBulb deSerializeTimerOfBulb(const char* string) {
    TimerOfBulb tbulb;
    sscanf(string, "tbulb %d %s %d %d %ld %ld %ld",
        &tbulb.id,
        tbulb.name,
        &tbulb.state,
        &tbulb.mainSwitch,
        &tbulb.activeTime,
        &tbulb.begin,
        &tbulb.end
    );
    return tbulb;
}

void printTimerOfBulb(const TimerOfBulb tbulb) {
    char strbegin[DATESIZE];
    char strend[DATESIZE];
    timeToStr(strbegin, tbulb.begin);
    timeToStr(strend, tbulb.end);

    char strstate[16];
    stateToStr(strstate, tbulb.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- mainSwitch: %s\n- activeTime: %ld seconds\n- begin: %s\n- end: %s\n\n",
        tbulb.name,
        tbulb.id,
        strstate,
        tbulb.mainSwitch ? "on" : "off",
        tbulb.activeTime,
        strbegin,
        strend
    );
}

// TIMER OF WINDOW

TimerOfWindow createTimerOfWindow(const Timer timer, const Window window) {
    TimerOfWindow twindow = {
        timer.id,
        "",
        window.state,
        window.openSwitch,
        window.closeSwitch,
        window.openTime,
        timer.begin,
        timer.end
    };
    strcpy(twindow.name, timer.name);
    return twindow;
}

void serializeTimerOfWindow(char* string, const TimerOfWindow twindow) {
    sprintf(string, "twindow %d %s %d %d %d %ld %ld %ld",
        twindow.id,
        twindow.name,
        twindow.state,
        twindow.openSwitch,
        twindow.closeSwitch,
        twindow.openTime,
        twindow.begin,
        twindow.end
    );
}

TimerOfWindow deSerializeTimerOfWindow(const char* string) {
    TimerOfWindow twindow;
    sscanf(string, "twindow %d %s %d %d %d %ld %ld %ld",
        &twindow.id,
        twindow.name,
        &twindow.state,
        &twindow.openSwitch,
        &twindow.closeSwitch,
        &twindow.openTime,
        &twindow.begin,
        &twindow.end
    );
    return twindow;
}

void printTimerOfWindow(const TimerOfWindow twindow) {
    char strbegin[DATESIZE];
    char strend[DATESIZE];
    timeToStr(strbegin, twindow.begin);
    timeToStr(strend, twindow.end);

    char strstate[16];
    stateToStr(strstate, twindow.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- openSwitch: %s\n- closeSwitch: %s\n- openTime: %ld seconds\n- begin: %s\n- end: %s\n\n",
        twindow.name,
        twindow.id,
        strstate,
        twindow.openSwitch ? "on" : "off",
        twindow.closeSwitch ? "on" : "off",
        twindow.openTime,
        strbegin,
        strend
    );
}

// TIMER OF FRIDGE

TimerOfFridge createTimerOfFridge(const Timer timer, const Fridge fridge) {
    TimerOfFridge tfridge = {
        timer.id,
        "",
        fridge.state,
        fridge.mainSwitch,
        fridge.thermostat,
        fridge.openTime,
        fridge.delay,
        fridge.perc,
        fridge.temp,
        timer.begin,
        timer.end
    };
    strcpy(tfridge.name, timer.name);
    return tfridge;
}

void serializeTimerOfFridge(char* string, const TimerOfFridge tfridge) {
    sprintf(string, "tfridge %d %s %d %d %d %ld %ld %d %d %ld %ld",
        tfridge.id,
        tfridge.name,
        tfridge.state,
        tfridge.mainSwitch,
        tfridge.thermostat,
        tfridge.openTime,
        tfridge.delay,
        tfridge.perc,
        tfridge.temp,
        tfridge.begin,
        tfridge.end
    );
}

TimerOfFridge deSerializeTimerOfFridge(const char* string) {
    TimerOfFridge tfridge;
    sscanf(string, "tfridge %d %s %d %d %d %ld %ld %d %d %ld %ld",
        &tfridge.id,
        tfridge.name,
        &tfridge.state,
        &tfridge.mainSwitch,
        &tfridge.thermostat,
        &tfridge.openTime,
        &tfridge.delay,
        &tfridge.perc,
        &tfridge.temp,
        &tfridge.begin,
        &tfridge.end
    );
    return tfridge;
}

void printTimerOfFridge(const TimerOfFridge tfridge) {
    char strbegin[DATESIZE];
    char strend[DATESIZE];
    timeToStr(strbegin, tfridge.begin);
    timeToStr(strend, tfridge.end);
    
    char strstate[16];
    stateToStr(strstate, tfridge.state);

    printf("\n%s:\n- id: %d\n- state: %s\n- mainSwitch: %s\n- thermostat: %d °C\n- openTime: %ld seconds\n- delay: %ld seconds\n- perc: %d%%\n- temp: %d °C\n- begin: %s\n- end: %s\n\n",
        tfridge.name,
        tfridge.id,
        strstate,
        tfridge.mainSwitch ? "on" : "off",
        tfridge.thermostat,
        tfridge.openTime,
        tfridge.delay,
        tfridge.perc,
        tfridge.temp,
        strbegin,
        strend
    );
}