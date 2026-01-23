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

    local commands="transform count distinct list-errors summarise-errors stats latest"
    
    # Common options for all commands
    local common_opts="-r --recursive -v -V -e --extension -d --depth -if --input-format --min-date --max-date"
    
    # Command-specific options
    local transform_opts="-o --output -of --output-format --tail --tail-column-value --use-prototype --not-empty --not-null --only-value --exclude-value --allowed-values --remove-errors --remove-whitespace --remove-empty-json --update-value --update-where-empty --unique --clean"
    local count_opts="-o --output -of --output-format -f --follow -b --by-column --by-day --by-week --by-month --by-year --tail --tail-column-value --not-empty --not-null --only-value --exclude-value --allowed-values --remove-errors --remove-empty-json --unique --clean"
    local distinct_opts="-c --counts -of --output-format --not-empty --not-null --only-value --exclude-value --allowed-values --after --before --remove-errors --remove-empty-json --clean --unique"
    local list_errors_opts="-o --output"
    local summarise_errors_opts="-o --output"
    local stats_opts="-c --column -f --follow --tail --tail-column-value -o --output --group-by --not-empty --not-null --only-value --exclude-value --allowed-values --remove-errors --remove-empty-json --unique --clean"
    local latest_opts="-n -of --output-format --tail --tail-column-value --not-empty --not-null --only-value --exclude-value --allowed-values --remove-errors --remove-empty-json --unique --clean"

    # Determine which command we're completing for
    local cmd=""
    local i
    for ((i=1; i < cword; i++)); do
        case "${words[i]}" in
            transform|count|distinct|list-errors|summarise-errors|stats|latest)
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
        --tail)
            # Suggest some common tail values
            COMPREPLY=($(compgen -W "10 50 100 500 1000" -- "$cur"))
            return
            ;;
        -if|--input-format)
            COMPREPLY=($(compgen -W "json csv" -- "$cur"))
            return
            ;;
        -of|--output-format)
            COMPREPLY=($(compgen -W "json csv rdata rds human plain" -- "$cur"))
            return
            ;;
        --not-empty|--not-null|--column|--group-by|-c)
            # Can't easily complete column names, leave empty
            return
            ;;
        --only-value|--exclude-value|--update-value|--update-where-empty|--tail-column-value)
            # Format is column:value, can't easily complete
            return
            ;;
        --allowed-values)
            # First arg is column name, second arg is values or file
            # Can't easily complete column names, leave empty
            return
            ;;
        --after|--before|--min-date|--max-date)
            # Can't complete dates, leave empty
            return
            ;;
        -n)
            # Suggest some common values for -n
            COMPREPLY=($(compgen -W "1 5 10 -1 -5 -10" -- "$cur"))
            return
            ;;
    esac

    # Complete based on command
    if [[ "$cur" == -* ]]; then
        case "$cmd" in
            transform)
                COMPREPLY=($(compgen -W "$common_opts $transform_opts" -- "$cur"))
                ;;
            count)
                COMPREPLY=($(compgen -W "$common_opts $count_opts" -- "$cur"))
                ;;
            distinct)
                COMPREPLY=($(compgen -W "$common_opts $distinct_opts" -- "$cur"))
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
            latest)
                COMPREPLY=($(compgen -W "$common_opts $latest_opts" -- "$cur"))
                ;;
        esac
    else
        # Complete file/directory names
        _filedir 2>/dev/null || COMPREPLY=($(compgen -f -- "$cur"))
    fi
}

complete -F _sensor_data sensor-data
