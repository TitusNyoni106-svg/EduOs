print("===== EduOS =====")
print("1. Start Kernel")
print("2. Run Scheduler")
print("3. Run Controller")
print("4. Exit")

choice = input("Choose option: ")

if choice == "1":
    print("Kernel starting...")

elif choice == "2":
    print("Scheduler running...")

elif choice == "3":
    print("Controller running...")

elif choice == "4":
    print("Exiting EduOS...")

else:
    print("Invalid choice")