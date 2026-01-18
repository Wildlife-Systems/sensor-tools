# Bash completion for sensor-data
# Install: source this file or copy to /etc/bash_completion.d/

_sensor_data() {
    local cur prev words cword
    
    # Use _init_completion if available, otherwise fall back to manual setup
    if declare -F _init_completion >/dev/null 2>&1; then
        _init_completion || return
    else
        COMPREPLY=()
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    local commands="transform list-errors summarise-errors stats"
    
    # Common options for all commands
    local common_opts="-r --recursive -v -V -e --extension -d --depth -f --format --min-date --max-date"
    
    # Command-specific options
    local transform_opts="-o --output -F --output-format --use-prototype --not-empty --only-value --exclude-value --remove-errors --remove-whitespace"
    local list_errors_opts="-o --output"
    local summarise_errors_opts="-o --output"
    local stats_opts="-o --output --column --group-by"

    # Determine which command we're completing for
    local cmd=""
    local i
    for ((i=1; i < cword; i++)); do
        case "${words[i]}" in
            transform|list-errors|summarise-errors|stats)
                cmd="${words[i]}"
                break
                ;;
        esac
    done

    # If no command yet, complete commands
    if [[ -z "$cmd" ]]; then
        if [[ "$cur" == -* ]]; then
            COMPREPLY=($(compgen -W "--help" -- "$cur"))
        else
            COMPREPLY=($(compgen -W "$commands" -- "$cur"))
        fi
        return
    fi

    # Complete options based on command
    case "$prev" in
        -o|--output)
            # Complete file names
            _filedir 2>/dev/null || COMPREPLY=($(compgen -f -- "$cur"))
            return
            ;;
        -e|--extension)
            # Common sensor file extensions
            COMPREPLY=($(compgen -W ".out .csv .json .log" -- "$cur"))
            return
            ;;
        -d|--depth)
            # Suggest some common depth values
            COMPREPLY=($(compgen -W "0 1 2 3 5 10" -- "$cur"))
            return
            ;;
        -f|--format)
            COMPREPLY=($(compgen -W "json csv" -- "$cur"))
            return
            ;;
        -F|--output-format)
            COMPREPLY=($(compgen -W "json csv" -- "$cur"))
            return
            ;;
        --not-empty|--column|--group-by)
            # Can't easily complete column names, leave empty
            return
            ;;
        --only-value)
            # Format is column:value, can't easily complete
            return
            ;;
        --min-date|--max-date)
            # Can't complete dates, leave empty
            return
            ;;
    esac

    # Complete based on command
    if [[ "$cur" == -* ]]; then
        case "$cmd" in
            transform)
                COMPREPLY=($(compgen -W "$common_opts $transform_opts" -- "$cur"))
                ;;
            list-errors)
                COMPREPLY=($(compgen -W "$common_opts $list_errors_opts" -- "$cur"))
                ;;
            summarise-errors)
                COMPREPLY=($(compgen -W "$common_opts $summarise_errors_opts" -- "$cur"))
                ;;
            stats)
                COMPREPLY=($(compgen -W "$common_opts $stats_opts" -- "$cur"))
                ;;
        esac
    else
        # Complete file/directory names
        _filedir 2>/dev/null || COMPREPLY=($(compgen -f -- "$cur"))
    fi
}

complete -o default -o bashdefault -F _sensor_data sensor-data
