set_last_mod_time_the_same_as_this_file="$1"
set_last_mod_time_for_this_file="$2"
touch -r "$set_last_mod_time_the_same_as_this_file" "$set_last_mod_time_for_this_file"
