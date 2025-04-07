#ifndef CLI_H
#define CLI_H

// Command-line interface (CLI) for the HeatStack application.
class CLI {
public:
    CLI();
    ~CLI();

    // Run the CLI.
    void run();

private:
    // Display help information.
    void showHelp() const;
};

#endif // CLI_H
