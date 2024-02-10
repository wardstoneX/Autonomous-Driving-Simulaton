from collections import namedtuple
import time
from scenario import ScenarioSetup


scneario = None
try:
    scenario = ScenarioSetup()
    scenario.initialize_scenario()
    
    while True:
        if scenario.control_data_receiver.control_data_queue:
            control_data = scenario.control_data_receiver.control_data_queue.get()
            
            if all(value == 0 for value in control_data):
                break
            print(control_data)
            print(scenario.main_vehicle.get_location().x)

            scenario.apply_control(control_data)
            
            if control_data.time > 0:
                time.sleep(control_data.time)
                scenario.brake()
                print(scenario.main_vehicle.get_location().x)
    
finally:
    scenario.clean_up()
    
    

    

