
# ---------------------------------------------------------------------------
# Pin definitions
# ---------------------------------------------------------------------------


#===============================================================================
#                            Clocks & system signals
#===============================================================================

#set_property -dict {PACKAGE_PIN C4 IOSTANDARD LVDS_25} [ get_ports clk_100mhz_clk_p ]
#set_property -dict {PACKAGE_PIN C3 IOSTANDARD LVDS_25} [ get_ports clk_100mhz_clk_n ]

set_property -dict {PACKAGE_PIN AH12} [ get_ports pci_refclk_clk_p ] ;# PCIE endpoint refclk#2
set_property -dict {PACKAGE_PIN AH11} [ get_ports pci_refclk_clk_n ]  


#create_clock -period 10.000 -name sysclk100   [get_ports clk_100mhz_clk_p]
#set_clock_groups -name group_sysclk100 -asynchronous -group [get_clocks sysclk100]

create_clock -period 10.000 -name pcie_sysclk [get_ports pci_refclk_clk_p]
set_clock_groups -name group_pcie_sysclk -asynchronous -group [get_clocks pcie_sysclk]

# Disable timing analysis for these pins
set_disable_timing [get_ports led_pci_link_up ]

#######################################
#  Miscellaneous
#######################################
set_property -dict {PACKAGE_PIN D1  IOSTANDARD LVCMOS33}  [get_ports { led_pci_link_up  }] ;# USER_LED9




#set_property  -dict {PACKAGE_PIN B6 IOSTANDARD LVCMOS33} [get_ports pb_rst_n]  ;# PB_SW0
#set_property  -dict {PACKAGE_PIN A3 IOSTANDARD LVCMOS33} [get_ports pb_go   ]  ;# PB_SW1
#set_property  -dict {PACKAGE_PIN B3 IOSTANDARD LVCMOS33} [get_ports pb_read ]  ;# PB_SW


#set_property  PACKAGE_PIN  B5    [get_ports {  led[0]                    }]
#set_property  PACKAGE_PIN  A5    [get_ports {  led[1]                    }]
#set_property  PACKAGE_PIN  A4    [get_ports {  led[2]                    }]
#set_property  PACKAGE_PIN  C5    [get_ports {  led[3]                    }]
#set_property  PACKAGE_PIN  C6    [get_ports {  led[4]                    }]
#set_property  PACKAGE_PIN  C1    [get_ports {  led[5]                    }]
#set_property  PACKAGE_PIN  D2    [get_ports {  led[6]                    }]
#set_property  PACKAGE_PIN  D3    [get_ports {  led[7]                    }]
#set_property  PACKAGE_PIN  D4    [get_ports {  led[8]                    }]
#set_property  PACKAGE_PIN  D1    [get_ports {  led[9]                    }]

