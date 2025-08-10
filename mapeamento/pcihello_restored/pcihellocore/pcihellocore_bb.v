
module pcihellocore (
	pcie_hard_ip_0_pcie_rstn_export,
	pcie_hard_ip_0_powerdown_pll_powerdown,
	pcie_hard_ip_0_powerdown_gxb_powerdown,
	pcie_hard_ip_0_refclk_export,
	pcie_hard_ip_0_rx_in_rx_datain_0,
	pcie_hard_ip_0_tx_out_tx_dataout_0,
	pio_0_lcd_external_connection_export,
	pio_1_display_l_external_connection_export,
	pio_2_display_r_external_connection_export,
	pio_3_switches_external_connection_export,
	pio_4_push_b_external_connection_export,
	pio_5_r_leds_external_connection_export,
	pio_6_g_leds_external_connection_export);	

	input		pcie_hard_ip_0_pcie_rstn_export;
	input		pcie_hard_ip_0_powerdown_pll_powerdown;
	input		pcie_hard_ip_0_powerdown_gxb_powerdown;
	input		pcie_hard_ip_0_refclk_export;
	input		pcie_hard_ip_0_rx_in_rx_datain_0;
	output		pcie_hard_ip_0_tx_out_tx_dataout_0;
	output	[31:0]	pio_0_lcd_external_connection_export;
	output	[31:0]	pio_1_display_l_external_connection_export;
	output	[31:0]	pio_2_display_r_external_connection_export;
	input	[31:0]	pio_3_switches_external_connection_export;
	input	[31:0]	pio_4_push_b_external_connection_export;
	output	[31:0]	pio_5_r_leds_external_connection_export;
	output	[31:0]	pio_6_g_leds_external_connection_export;
endmodule
