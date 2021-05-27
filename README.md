# FreeRTOS-Traffic-Lights-Controller
FreeRTOS traffic light controller with pedestrians crossing on tm4c123 board

Youtube Video: https://youtu.be/wkStpFIdqy0

The aim of this project is control a junction where two roads cross each other north road and east road.
We have normal mode that alters between north road and east road.
Pedestrian crossing mode, when the push button in the top right in pressed, the system waits for the current green lights to finish then closes all roads for pedestrians to cross, then it continues the normal mode.
Train crossing mode, when either push buttons in the bottom is pressed, a siren is started, the gate is closed and all roads are closed, this is the highest priority mode.
