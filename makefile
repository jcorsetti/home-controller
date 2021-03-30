CC=gcc
CFLAGS=-std=gnu90 -Isrc/libs/

# directory dei sorgenti
SPATH=./src
# directory delle librerie
LPATH=./src/libs

help:
	@cat helpFile.txt | less

build: controller bulb fridge window hub timer manual

controller: $(SPATH)/controller.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o controller $(SPATH)/controller.o $(LPATH)//messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

manual: $(SPATH)/manual.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o manual $(SPATH)/manual.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

hub: $(SPATH)/hub.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o hub $(SPATH)/hub.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

timer: $(SPATH)/timer.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o timer $(SPATH)/timer.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

bulb: $(SPATH)/bulb.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o bulb $(SPATH)/bulb.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

window: $(SPATH)/window.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o window $(SPATH)/window.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

fridge: $(SPATH)/fridge.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o
	$(CC) -o fridge $(SPATH)/fridge.o $(LPATH)/messages.o $(LPATH)/devices.o $(LPATH)/utility.o $(LPATH)/devList.o

# rimuove i file oggetto ed i file temporanei
clean:
	rm -f $(SPATH)/controller.o
	rm -f $(SPATH)/manual.o
	rm -f $(SPATH)/hub.o
	rm -f $(SPATH)/timer.o
	rm -f $(SPATH)/bulb.o
	rm -f $(SPATH)/window.o
	rm -f $(SPATH)/fridge.o
	rm -f $(LPATH)/devices.o
	rm -f $(LPATH)/devList.o
	rm -f $(LPATH)/messages.o
	rm -f $(LPATH)/utility.o
	rm -rf src/tmp/*
