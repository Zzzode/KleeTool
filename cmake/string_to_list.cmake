function(string_to_list s output_var)
  string(REPLACE " " ";" _output "${s}")
  set(${output_var} ${_output} PARENT_SCOPE)
endfunction()
