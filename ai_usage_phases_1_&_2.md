PHASE 1:
Tool used: Gemini
1.First prompt I gave was "Help me write this filter function for my project"
I attached a file of my full code, containing all functions except for filter,
as well as a pdf version of the document containing the project details.
These are the generated functions:
int parse_condition(const char *cond_str, char *field, char *op, char *val) {
    // Expected format: "field op value" (e.g., "sev_lvl > 1")
    if (sscanf(cond_str, "%s %s %s", field, op, val) != 3) {
        return 0; // Failed to parse
    }
    return 1;
}

// AI-GENERATED: Function to check if a report matches the parsed condition
int match_condition(Report r, const char *field, const char *op, const char *val) {
    if (strcmp(field, "sev_lvl") == 0) {
        int v = atoi(val);
        if (strcmp(op, "==") == 0) return r.sev_lvl == v;
        if (strcmp(op, ">") == 0)  return r.sev_lvl > v;
        if (strcmp(op, "<") == 0)  return r.sev_lvl < v;
    }
    else if (strcmp(field, "issue_categ") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r.issue_categ, val) == 0;
    }
    else if (strcmp(field, "id") == 0) {
        int v = atoi(val);
        if (strcmp(op, "==") == 0) return r.id == v;
    }
    else if (strcmp(field, "ins_name") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r.ins_name, val) == 0;
    }
    return 0; // No match or unknown field
}

Mistake: Gemini interpreted the format as field op value, with spaces in between, when 
the format is actually field:op:value, clearly specified in the document. I made the change accordingly.
Mistake: In the match_condition function, instead of checking for the specified keywords: severity, category, inspector, timestamp, it checked for matches with the names
I gave to the report struct fields, like sev_lvl, issue_categ, ins_name and instead of the timestamp it checked for the id. It also did not check for
all the listed permitted operators ==, !=, <, <=, >, >=, only for ==, > and <. It seems like Gemini had a hard time following all instructions in the document,
only focusing on the larger details. I manually changed the function to include the necessary comparisons.
2.In the add_report function, the user has to enter the details about the report in the console.
Gemini suggested adding calls such as "getchar();" and "new_report.desc[strcspn(new_report.desc, "\n")] = 0;"
in order to remove the endline character read and stored by fgets(), because it could be problematic for my filter
function: will break your filter comparisons later because "road\n" does not equal "road".
3.I also used Gemini to generate inputs, which I have stored in the input.txt file, with this
prompt:give me a series of prompts to test my program: 5 additions, one list, one view of the 2nd report, one update treshold,  
one filter, removal of the first,last and middle reports and one final list

PHASE 2:
I used Gemini to debug my code. I gave it the monitor_reports.c file and the description of the monitor_reports part of phase 2, alongside the
functions we are recommended to use (or forbidden to), asking it to check why my program ends abruptly.
The big problem: my program was reaching return 0 before receiving any signal. 
The suggested fix: 
while (!end_process) {
    pause(); 
}
You need a while loop at the end of main so the process doesn't just reach return 0 and die immediately.
Using pause() is much better than a busy-wait loop like while(1);
A busy-wait would spike your CPU usage to 100%, whereas pause() tells the OS to wake the process up only when a signal arrives.
This way, the program will only continue past the loop after it receives an ending signal(SIGINT in this case).

Other suggestions were using write() instead of printf in my signal handling function, because printf() uses a buffer that can lead to deadlocks if a signal interrupts another I/O call,
whereas write() safe to call even if the signal interrupts another I/O operation, and changing my global int variable end_process to volatile sig_atomic_t. This seems a bit overkill
as the int variable works just fine, it feels like a higher level modification that does not reflect my level of understanding OS.
