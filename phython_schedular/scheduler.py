print("EduOS Scheduler Initialized")

process_queue = ["Process A", "Process B", "Process C"]

def start_scheduler():
    print("Starting scheduler...")
    
    for process in process_queue:
        print("Running:", process)

start_scheduler()