#/usr/bin/env bash
_uno_completions() 
{
    local cur prev opts base
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    #  The basic options to complete.
    opts="-mechanism -strategy -feasibility -subproblem"

    #  Complete the arguments to some of the basic commands.
    case "${prev}" in
        -mechanism)
			local mechanisms="TR LS"
            COMPREPLY=( $(compgen -W "${mechanisms}" -- ${cur}) )
            return 0
            ;;
        -strategy)
			local strategies="l1-penalty filter nonmonotone-filter"
            COMPREPLY=( $(compgen -W "${strategies}" -- ${cur}) )
            return 0
            ;;
        -feasibility)
			local feasibility="feasibility-restoration l1-relaxation"
            COMPREPLY=( $(compgen -W "${feasibility}" -- ${cur}) )
            return 0
            ;;
		-subproblem)
			local subproblems="SQP SLP Sl1QP IPM"
            COMPREPLY=( $(compgen -W "${subproblems}" -- ${cur}) )
            return 0
            ;;
        *)
        ;;
    esac

	if [[ ${cur} == -* ]] ; then
		COMPREPLY=($(compgen -W "${opts}" -- ${cur}))
		return 0
	else
		_filedir
    fi
}
complete -F _uno_completions uno
