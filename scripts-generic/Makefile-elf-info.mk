# Represents an empty white space - we need it for extracting the elf entry address from readelf output
SPACE := $() $()

.PHONY: all

# Default build
all: release

release: rel_read_elf_info rel_extract_load_offset rel_extract_load_virtaddr rel_extract_load_physaddr rel_extract_entry_addr

rel_read_elf_info:
	$(eval REL_ELF_INFO := $(shell arm-none-eabi-readelf -l $(APP_OUT_PATH)/Release/$(APP_PROGRAM_NAME1).elf))
	$(info ELF info: $(REL_ELF_INFO))

# Extracts the load offset
rel_extract_load_offset:
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(REL_ELF_INFO)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD_%,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(word 1,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD_,,$(REL_ELF_LOAD_TEXT)))
	$(info ELF-load-address (OFFSET)  : $(REL_ELF_LOAD_TEXT))

# Extracts the load virtual address
rel_extract_load_virtaddr:
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(REL_ELF_INFO)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD_%,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(word 1,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD_,,$(REL_ELF_LOAD_TEXT)))
	$(info ELF-load-address (VIRTADDR): $(REL_ELF_LOAD_TEXT))

# Extracts the load physical address
rel_extract_load_physaddr:
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(REL_ELF_INFO)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(filter LOAD_%,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(word 1,$(REL_ELF_LOAD_TEXT)))
	$(eval REL_ELF_LOAD_TEXT := $(subst LOAD_,,$(REL_ELF_LOAD_TEXT)))
	$(info ELF-load-address (PHYSADDR): $(REL_ELF_LOAD_TEXT))

rel_extract_entry_addr:
	$(eval REL_ELF_ENTRY_TEXT := $(filter Entry 0x%,$(REL_ELF_INFO)))
	$(eval REL_ELF_ENTRY_TEXT := $(subst Entry$(SPACE),Entry_,$(REL_ELF_ENTRY_TEXT)))
	$(eval REL_ELF_ENTRY_TEXT := $(filter Entry_%,$(REL_ELF_ENTRY_TEXT)))
	$(eval REL_ELF_ENTRY_TEXT := $(subst Entry_,,$(REL_ELF_ENTRY_TEXT)))
	$(info ELF-entry-address (ADDR)   : $(REL_ELF_ENTRY_TEXT))

debug: dbg_read_elf_info dbg_extract_load_offset dbg_extract_load_virtaddr dbg_extract_load_physaddr dbg_extract_entry_addr

dbg_read_elf_info:
	$(eval DBG_ELF_INFO := $(shell arm-none-eabi-readelf -l $(APP_OUT_PATH)/Debug/$(APP_PROGRAM_NAME1).elf))
	$(info ELF info: $(DBG_ELF_INFO))

# Extracts the load offset
dbg_extract_load_offset:
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(DBG_ELF_INFO)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD_%,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(word 1,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD_,,$(DBG_ELF_LOAD_TEXT)))
	$(info ELF-load-address (OFFSET)  : $(DBG_ELF_LOAD_TEXT))

# Extracts the load virtual address
dbg_extract_load_virtaddr:
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(DBG_ELF_INFO)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD_%,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(word 1,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD_,,$(DBG_ELF_LOAD_TEXT)))
	$(info ELF-load-address (VIRTADDR): $(DBG_ELF_LOAD_TEXT))

# Extracts the load physical address
dbg_extract_load_physaddr:
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD 0x%,$(DBG_ELF_INFO)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(patsubst LOAD_%,LOAD,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD$(SPACE),LOAD_,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(filter LOAD_%,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(word 1,$(DBG_ELF_LOAD_TEXT)))
	$(eval DBG_ELF_LOAD_TEXT := $(subst LOAD_,,$(DBG_ELF_LOAD_TEXT)))
	$(info ELF-load-address (PHYSADDR): $(DBG_ELF_LOAD_TEXT))

dbg_extract_entry_addr:
	$(eval DBG_ELF_ENTRY_TEXT := $(filter Entry 0x%,$(DBG_ELF_INFO)))
	$(eval DBG_ELF_ENTRY_TEXT := $(subst Entry$(SPACE),Entry_,$(DBG_ELF_ENTRY_TEXT)))
	$(eval DBG_ELF_ENTRY_TEXT := $(filter Entry_%,$(DBG_ELF_ENTRY_TEXT)))
	$(eval DBG_ELF_ENTRY_TEXT := $(subst Entry_,,$(DBG_ELF_ENTRY_TEXT)))
	$(info ELF-entry-address (ADDR)   : $(DBG_ELF_ENTRY_TEXT))
