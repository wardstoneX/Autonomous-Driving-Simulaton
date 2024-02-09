from collections import namedtuple
import time
from scenario import ScenarioSetup



try:
    scenario = ScenarioSetup()
    scenario.initialize_scenario()
    
    while True:
        if scenario.control_data_receiver.control_data_queue:
            control_data = scenario.control_data_receiver.control_data_queue.get()
            
            if all(value == 0 for value in control_data):
                break
            
            scenario.apply_control(control_data)
            
            if control_data.time > 0:
                time.sleep(control_data.time)
                scenario.brake()
    
finally:
    scenario.clean_up()
    
    

    

