file_path = '/src/rpm'

def print_rpm_data():
    try:
        with open(file_path, 'r') as f:
            for line in f:
                print(line.strip())
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Error reading from file: {e}")

if __name__ == "__main__":
    print_rpm_data()
