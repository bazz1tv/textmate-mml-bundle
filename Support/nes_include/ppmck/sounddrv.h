;-------------------------------------------------------------------------------
;macros and misc sub routines
;-------------------------------------------------------------------------------
;indirect_lda
;statement
;	indirect_lda	hoge_add_low	;hoge_add_low is not zero page address
;is same as:
;	lda	[hoge_add_low,x]
indirect_lda	.macro
	lda	\1,x		;hoge_add_low
	sta	<zero_add_low
	lda	\1+1,x		;hoge_add_high
	sta	<zero_add_high
	stx	<x_escape
	ldx	#$00
	lda	[zero_add_low,x]
	ldx	<x_escape
	.endm
;--------------------------------------	
reg400x:	; X = ���݂�ch<<1 , Y = X<<1
	lda	channel_sel
	asl	a
	tax
	asl	a
	tay
	rts

;-------------------------------------------------------------------------------
;initialize routine
;-------------------------------------------------------------------------------
INITIAL_WAIT_FRM = $00 ;�ŏ��ɂ��̃t���[���������E�F�C�g
;���������[�`��
sound_init:
	.if TOTAL_SONGS > 1
	pha
	.endif
	lda	#$00
	ldx	#$00
.memclear
	sta	$0000,x
	sta	$0200,x
	sta	$0300,x
	sta	$0400,x
	sta	$0500,x
	sta	$0600,x
	sta	$0700,x
	inx
	bne	.memclear

	lda	#INITIAL_WAIT_FRM
	sta	initial_wait

	lda	#$0f		;��������������
	sta	$4015		;�`�����l���g�p�t���O
	lda	#$08		
	sta	$4001		;��`�go2a�ȉ��΍�
	sta	$4005

	.if	SOUND_GENERATOR & __FME7
	jsr	fme7_sound_init
	.endif

	.if	SOUND_GENERATOR & __MMC5
	jsr	mmc5_sound_init
	.endif

	.if	SOUND_GENERATOR & __FDS
	jsr	fds_sound_init
	.endif
	
	.if	SOUND_GENERATOR & __N106
	jsr	n106_sound_init
	.endif

	.if	SOUND_GENERATOR & __VRC6
	jsr	vrc6_sound_init
	.endif

	.if TOTAL_SONGS > 1
		pla
		asl	a
		tax
		
		lda	song_addr_table,x
		sta	<start_add_lsb
		lda	song_addr_table+1,x
		sta	<start_add_lsb+1
		
		.if (ALLOW_BANK_SWITCH)
			lda	song_bank_table,x
			sta	<start_bank
			lda	song_bank_table+1,x
			sta	<start_bank+1
		.endif
	
	.endif
	
	lda	#$00
	sta	channel_sel
sound_channel_set:
	lda	channel_sel
	cmp	#PTR_TRACK_END		;�I���H
	beq	sound_init_end
	
	lda	channel_sel
	
	
	.if TOTAL_SONGS > 1
		.if (ALLOW_BANK_SWITCH)
			tay				; y = ch; x = ch<<1;
			asl	a
			tax
			lda	[start_bank],y
			sta	sound_bank,x
			
			txa				; x = y = ch<<1;
			tay
		.else
			asl	a			; x = y = ch<<1;
			tax
			tay
		.endif
				
		lda	[start_add_lsb],y
		sta	<sound_add_low,x	;�f�[�^�J�n�ʒu��������
		iny
		lda	[start_add_lsb],y
		sta	<sound_add_low+1,x	;�f�[�^�J�n�ʒu��������
	.else
	
		tay				; x = ch<<1; y = ch;
		asl	a
		tax
		

		.if (ALLOW_BANK_SWITCH)
			lda	song_000_bank_table,y
			sta	sound_bank,x
		.endif
		
		lda	song_000_track_table,x
		sta	<sound_add_low,x	;�f�[�^�J�n�ʒu��������
		lda	song_000_track_table+1,x
		sta	<sound_add_low+1,x	;�f�[�^�J�n�ʒu��������

	.endif
	; x = ch<<1; y = ?
	
	lda	#$00
	sta	effect_flag,x
	lda	#$01
	sta	sound_counter,x
	
	inc	channel_sel
	jmp	sound_channel_set
sound_init_end:
	rts

;-------------------------------------------------------------------------------
;main routine
;-------------------------------------------------------------------------------
sound_driver_start:

	lda	initial_wait
	beq	.gogo
	dec	initial_wait
	rts
.gogo

	lda	#$00
	sta	channel_sel

internal_return:
	jsr	sound_internal
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	cmp	#$04
	bne	internal_return		;�߂�

;	.if	DPCMON
sound_dpcm_part:
	jsr	sound_dpcm
;	.endif
	inc	channel_sel		;���̃`�����l����ݒ肵��

	.if	SOUND_GENERATOR & __FDS
	jsr	sound_fds		;FDS�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	.endif

	.if	SOUND_GENERATOR & __VRC7
vrc7_return:
	jsr	sound_vrc7		;vrc7�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	sec
	sbc	#PTRVRC7
	cmp	#$06			;vrc7�͏I��肩�H
	bne	vrc7_return		;�܂��Ȃ�߂�
	.endif

	.if	SOUND_GENERATOR & __VRC6
vrc6_return:
	jsr	sound_vrc6		;vrc6�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	sec
	sbc	#PTRVRC6
	cmp	#$03			;vrc6�͏I��肩�H
	bne	vrc6_return		;�܂��Ȃ�߂�
	.endif

	.if	SOUND_GENERATOR & __N106
.rept:
	jsr	sound_n106		;n106�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	sec
	sbc	#PTRN106
	cmp	n106_channel		;n106�͏I��肩�H
	bne	.rept			;�܂��Ȃ�߂�
	.endif

	.if	SOUND_GENERATOR & __FME7
fme7_return:
	jsr	sound_fme7		;fme7�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	sec
	sbc	#PTRFME7
	cmp	#$03			;fme7�͏I��肩�H
	bne	fme7_return		;�܂��Ȃ�߂�
	.endif

	.if	SOUND_GENERATOR & __MMC5
mmc5_return:
	jsr	sound_mmc5		;mmc5�s���Ă���
	inc	channel_sel		;���̃`�����l����ݒ肵��
	lda	channel_sel
	sec
	sbc	#PTRMMC5
	cmp	#$02			;mmc5�͏I��肩�H
	bne	mmc5_return		;�܂��Ȃ�߂�
	.endif

	rts

;------------------------------------------------------------------------------
;command read sub routines
;------------------------------------------------------------------------------
sound_data_address:
	inc	<sound_add_low,x	;�f�[�^�A�h���X�{�P
	bne	return2			;�ʂ��オ������
sound_data_address_inc_high
	inc	<sound_add_high,x	;�f�[�^�A�h���X�S�̈ʁi��j�{�P
return2:
	rts

sound_data_address_add_a:
	clc
	adc	<sound_add_low,x
	sta	<sound_add_low,x
	bcs	sound_data_address_inc_high
	rts
;-------------------------------------------------------------------------------
change_bank:
;�o���N��Reg.A�ɕς��܂��`
;�ύX�����o���N�A�h���X�̓o���N�R���g���[���ɂ��
;���݂�NSF�̂݁B
	if (ALLOW_BANK_SWITCH)
;�o���N�؂�ւ��ł���condition: A <= BANK_MAX_IN_4KB
;i.e. A < BANK_MAX_IN_4KB + 1
;i.e. A - (BANK_MAX_IN_4KB+1) < 0
;i.e. NOT ( A - (BANK_MAX_IN_4KB+1) >= 0 )
;skip����condition: A - (BANK_MAX_IN_4KB+1) >= 0
	cmp	#BANK_MAX_IN_4KB+1
	bcs	.avoidbankswitch
	sta	$5ffa ; A000h-AFFFh
	clc
	adc	#$01
	cmp	#BANK_MAX_IN_4KB+1
	bcs	.avoidbankswitch
	sta	$5ffb ; B000h-BFFFh
.avoidbankswitch
	endif
	rts

;-------------------------------------------------------------------------------
; ���s�[�g�I���R�}���h
;
; channel_loop++;
; if (channel_loop == <num>) {
;   channel_loop = 0;
;   �c��̃p�����[�^��������adr�����ɐi�߂�;
; } else {
;   0xee�R�}���h�Ɠ�������;
; }
loop_sub:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;�J��Ԃ���
	beq	loop_end
	jsr	sound_data_address
	jmp	bank_address_change
loop_end:
	lda	#$00
	sta	channel_loop,x
loop_esc_through			;loop_sub2������ł���
	lda	#$04
	jsr	sound_data_address_add_a
	rts				;�����܂�
;-----------
; ���s�[�g�r������
;
; channel_loop++;
; if (channel_loop == <num>) {
;   channel_loop = 0;
;   0xee�R�}���h�Ɠ�������;
; } else {
;   �c��̃p�����[�^��������adr�����ɐi�߂�;
; }

loop_sub2:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;�J��Ԃ���
	bne	loop_esc_through
	lda	#$00
	sta	channel_loop,x
	jsr	sound_data_address
	jmp	bank_address_change
;-------------------------------------------------------------------------------
;�o���N�Z�b�g (goto�R�}���h�Bbank, adr_low, adr_high)
data_bank_addr:
	jsr	sound_data_address
bank_address_change:
	if (ALLOW_BANK_SWITCH)
	lda	[sound_add_low,x]
	sta	sound_bank,x
	endif

	jsr	sound_data_address
	lda	[sound_add_low,x]
	pha
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<sound_add_high,x
	pla
	sta	<sound_add_low,x	;�V�����A�h���X������

	rts
;-------------------------------------------------------------------------------
;data_end_sub:
;	ldy	channel_sel
;	
;	if (ALLOW_BANK_SWITCH)
;	lda	loop_point_bank,y
;	sta	sound_bank,x
;	endif
;	
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;���[�v�J�n�ʒu�������� Low
;	inx
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;���[�v�J�n�ʒu�������� High
;	rts
;-------------------------------------------------------------------------------
volume_sub:
	lda	effect_flag,x
	ora	#%00000001
	sta	effect_flag,x		;�\�t�g�G���x�L���w��

;	lda	#$00
;	sta	register_low,x		;�ʏ�{�����[�����[����

	lda	temporary
	sta	softenve_sel,x
	asl	a
	tay
	lda	softenve_table,y	;�\�t�g�G���x�f�[�^�A�h���X�ݒ�
	sta	soft_add_low,x
	iny
	lda	softenve_table,y
	sta	soft_add_high,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
lfo_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	lfo_data_set

	jsr	reg400x
	lda	effect_flag,x
	and	#%10001111		;LFO��������
	sta	effect_flag,x
	jsr	sound_data_address
	rts
lfo_data_set:
	asl	a
	asl	a
	sta	lfo_data873,x

	tay
	lda	channel_sel
	asl	a
	tax
	lda	lfo_data,y
	sta	lfo_count,x		;�f�B���C�Z�b�g
	sta	lfo_start_count,x
	iny
	lda	lfo_data,y
	sta	lfo_revers,x		;�X�s�[�h�Z�b�g
	sta	lfo_revers_count,x
	iny
	lda	lfo_data,y
	sta	lfo_depth,x		;�f�v�X�Z�b�g
	iny
	lda	lfo_data,y
	sta	lfo_harf_time,x
	sta	lfo_harf_count,x		;1/2�J�E���^�Z�b�g

	jsr	warizan_start

	lda	channel_sel		;�Ȃ����̏��������Ă��邩�Ƃ�����
	sec				;���������Ɗg��������+-���t������ł���
	sbc	#$05
	bcc	urararara2


	lda	effect_flag,x
	ora	#%00110000
	sta	effect_flag,x
	jmp	ittoke2
urararara2:
	lda	effect_flag,x
	and	#%11011111		;�g�`�|����
	ora	#%00010000		;LFO�L���t���O�Z�b�g
	sta	effect_flag,x
ittoke2:
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
detune_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	detune_data_set

	jsr	reg400x
	lda	effect_flag,x
	and	#%01111111		;detune��������
	sta	effect_flag,x
	jsr	sound_data_address
	rts
detune_data_set:
	pha
	jsr	reg400x
	pla
	tay
	sta	detune_dat,x
	lda	effect_flag,x
	ora	#%10000000		;detune�L������
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
pitch_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	pitch_enverope_part
	lda	effect_flag,x
	and	#%11111101
	sta	effect_flag,x
	jsr	sound_data_address
	rts

pitch_enverope_part:
	sta	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_table,y
	sta	pitch_add_low,x
	iny
	lda	pitchenve_table,y
	sta	pitch_add_high,x
	lda	effect_flag,x
	ora	#%00000010
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
arpeggio_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	arpeggio_part

	lda	effect_flag,x
	and	#%11110111
	sta	effect_flag,x
	jsr	sound_data_address
	rts

arpeggio_part:
	sta	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_table,y
	sta	arpe_add_low,x
	iny
	lda	arpeggio_table,y
	sta	arpe_add_high,x

	lda	effect_flag,x
	ora	#%00001000
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
direct_freq_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_low,x		;Low
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_high,x		;High
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x			;Counter
	jsr	sound_data_address
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
y_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<reg_add_low
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<reg_add_high
	jsr	sound_data_address
	lda	[sound_add_low,x]
	ldx	#$00
	sta	[reg_add_low,x]
	jsr	reg400x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
wait_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x
	jsr	sound_data_address

	rts
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;effect sub routines
;-------------------------------------------------------------------------------
detune_write_sub:
	lda	effect_flag,x
	and	#%10000000
	bne	detune_part
	rts

detune_part:
	lda	detune_dat,x
	and	#%10000000
	bne	detune_minus

detune_plus:
	lda	detune_dat,x
	clc
	adc	sound_freq_low,x
	sta	sound_freq_low,x
	bcs	mid_plus
	rts
mid_plus:
	inc	sound_freq_high,x
	bne	n106_high_through
	inc	sound_freq_n106,x
n106_high_through:
	rts

detune_minus:
	lda	detune_dat,x
	and	#%01111111
	sta	detune_tmp
	lda	sound_freq_low,x
	sec
	sbc	detune_tmp
	sta	sound_freq_low,x
	bcc	mid_minus
	rts
mid_minus:
	lda	sound_freq_high,x
	beq	.borrow
.no_borrow
	dec	sound_freq_high,x
	rts
.borrow
	dec	sound_freq_high,x
	dec	sound_freq_n106,x
	rts
;----------------------------------------------
sound_software_enverope:
	jsr	volume_enve_sub
	sta	register_low,x
	ora	register_high,x		;���F�f�[�^�i���4bit�j�Ɖ���4bit�ő����Z
	sta	$4000,y			;�������݁`
	jsr	enverope_address	;�A�h���X����₵��
	rts				;�����܂�

volume_enve_sub:
	jsr	reg400x

	indirect_lda	soft_add_low		;�G���x���[�v�f�[�^�ǂݍ���
	cmp	#$ff			;�Ōォ�ǁ[��
	beq	return3			;�Ō�Ȃ烋�[�v������
	rts

return3:
	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_lp_table,y
	sta	soft_add_low,x
	iny
	lda	softenve_lp_table,y
	sta	soft_add_high,x
	jmp	volume_enve_sub
;-------------------------------------------------------------------------------
enverope_address:
	inc	soft_add_low,x
	bne	return5
	inc	soft_add_high,x
return5:
	rts
;-------------------------------------------------------------------------------
sound_duty_enverope:
	jsr	reg400x

	lda	channel_sel
	cmp	#$02
	beq	return21		;�O�p�g�Ȃ��΂��`

	indirect_lda	duty_add_low		;�G���x���[�v�f�[�^�ǂݍ���
	cmp	#$ff			;�Ōォ�ǁ[��
	beq	return22		;�Ō�Ȃ炻�̂܂܂����܂�
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	ora	#%00110000		;hardware envelope & ... disable
	sta	register_high,x
	ora	register_low,x		;���F�f�[�^�i���4bit�j�Ɖ���4bit�ő����Z
	sta	$4000,y			;�������݁`
	jsr	duty_enverope_address	;�A�h���X����₵��
return21:
	rts				;�����܂�

return22:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	iny
	lda	dutyenve_lp_table,y
	sta	duty_add_high,x
	jmp	sound_duty_enverope

;-------------------------------------------------------------------------------
duty_enverope_address:
	inc	duty_add_low,x
	bne	return23
	inc	duty_add_high,x
return23:
	rts
;--------------------------------------	
sound_lfo:
	lda	sound_freq_high,x
	sta	temporary

	jsr	lfo_sub

	lda	sound_freq_low,x
	sta	$4002,y			;�@�@���ݒl�����W�X�^�ɃZ�b�g
	lda	sound_freq_high,x
	cmp	temporary
	beq	end4
	sta	$4003,y
end4:
	rts				;�����܂�
;-------------------------------------------------------------------------------
lfo_sub:
	jsr	reg400x
	lda	lfo_start_count,x
	beq	lfo_start
	dec	lfo_start_count,x
	rts
lfo_start:

	lda	effect_flag,x
	and	#%01000000
	bne	hanbun

	lda	lfo_harf_count,x
	beq	lfo_start_2

	dec	lfo_harf_count,x
	bne	lfo_start_2

	lda	effect_flag,x
	ora	#%01000000
	sta	effect_flag,x
hanbun:
	jsr	lfo_set_2
	lsr	lfo_revers,x
	lsr	lfo_revers_count,x
	jsr	warizan

lfo_start_2:
	asl	lfo_revers,x
	lda	lfo_revers_count,x	;���݂̒l��ǂݍ����
	cmp	lfo_revers,x
	beq	lfo_revers_set		;�K�萔�ɒB���Ă����甽�]�Z�b�g
	jmp	lfo_depth_set		;�B���Ă��Ȃ���΃f�v�X������

lfo_revers_set:
	lda	#$00			;
	sta	lfo_revers_count,x	;���]�J�E���^������
	lda	effect_flag,x		;�G�t�F�N�g�ǂݍ����
	pha				;�ꎞ�ޔ�
	and	#%00100000
	bne	lfo_revers_p

	pla
	ora	#%00100000
	sta	effect_flag,x
	jmp	lfo_depth_set
lfo_revers_p:
	pla
	and	#%11011111
	sta	effect_flag,x

lfo_depth_set:
	lsr	lfo_revers,x
	lda	lfo_depth_count,x	;���ԓǂݍ���
	cmp	lfo_adc_sbc_time,x	;���݂̃J�E���^�Ɣ�r
	beq	lfo_depth_work		;��v���Ă���΃f�v�X������
	jmp	lfo_count_plus		;�܂��Ȃ�J�E���^�v���X��

lfo_depth_work:
	lda	#$00			;
	sta	lfo_depth_count,x	;�f�v�X�J�E���^������
	lda	effect_flag,x		;�{���|��
	and	#%00100000
	bne	lfo_depth_plus

lfo_depth_minus:
	lda	sound_freq_low,x	;�f�v�X�ǂݍ���
	sec
	sbc	lfo_depth,x		;����
	sta	sound_freq_low,x
	bcc	lfo_high_minus		;��������
	jmp	lfo_count_plus
lfo_high_minus:
	lda	sound_freq_high,x
	beq	.borrow			;1�������O�Ƀ[���������炻�̏�̌��ɂ����؂蔭��
.no_borrow
	dec	sound_freq_high,x
	jmp	lfo_count_plus
.borrow
	dec	sound_freq_high,x
	dec	sound_freq_n106,x
	jmp	lfo_count_plus

lfo_depth_plus:
	lda	sound_freq_low,x	;�f�v�X�ǂݍ���
	clc
	adc	lfo_depth,x		;����
	sta	sound_freq_low,x
	bcs	lfo_high_plus		;��������
	jmp	lfo_count_plus
lfo_high_plus:
	inc	sound_freq_high,x	;������
	bne	lfo_count_plus
	inc	sound_freq_n106,x

lfo_count_plus:
	inc	lfo_revers_count,x	;�J�E���^�����Ă��I��
	inc	lfo_depth_count,x
	lda	effect_flag,x
	and	#%01000000
	beq	endend
	jsr	lfo_set_2
	jsr	warizan
endend:
	rts

;-------------------------------------------------------------------------------
lfo_set_2:
	lda	lfo_data873,x
	tay
	lda	channel_sel
	asl	a
	tax
	iny
	lda	lfo_data,y
	sta	lfo_revers,x		;�X�s�[�h�Z�b�g
	sta	lfo_revers_count,x
	iny
	lda	lfo_data,y
	sta	lfo_depth,x		;�f�v�X�Z�b�g
	rts
;-------
warizan_start:
	lda	#$00
	sta	kotae
	lda	lfo_revers,x
	cmp	lfo_depth,x
	beq	plus_one
	bmi	depth_wari
revers_wari:
	lda	lfo_depth,x
	sta	waru
	lda	lfo_revers,x
	jsr	warizan
	lda	kotae
	sta	lfo_adc_sbc_time,x
	sta	lfo_depth_count,x
	lda	#$01
	sta	lfo_depth,x
	rts
depth_wari:
	lda	lfo_revers,x
	sta	waru
	lda	lfo_depth,x
	jsr	warizan
	lda	kotae
	sta	lfo_depth,x
	lda	#$01
	sta	lfo_adc_sbc_time,x
	sta	lfo_depth_count,x
	rts
plus_one:
	lda	#$01
	sta	lfo_depth,x
	sta	lfo_adc_sbc_time,x
	sta	lfo_depth_count,x
	rts
warizan:
	inc	kotae
	sec
	sbc	waru
	beq	warizan_end
	bcc	warizan_end
	jmp	warizan
warizan_end:
	rts

;-------------------------------------------------------------------------------
sound_pitch_enverope:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
pitch_write:
	lda	sound_freq_low,x
	sta	$4002,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	end3
	sta	$4003,y
end3:
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
pitch_sub:
	jsr	reg400x
	indirect_lda	pitch_add_low	
	cmp	#$ff
	beq	return62

	and	#%10000000
;	cmp	#%10000000
	bne	pitch_plus

	indirect_lda	pitch_add_low	
	clc
	adc	sound_freq_low,x
	sta	sound_freq_low,x	;low����������
	bcs	freq_high_plus_2
	rts
freq_high_plus_2:
	inc	sound_freq_high,x	;high���{�P
	bne	rreturn
	inc	sound_freq_n106,x
rreturn
	rts				;���W�X�^�������݂�GO!
pitch_plus:
	indirect_lda	pitch_add_low	
	and	#%01111111
	sta	pitch_tmp
	sec
	lda	sound_freq_low,x
	sbc	pitch_tmp
	sta	sound_freq_low,x
	bcc	freq_high_minus_2
	rts
freq_high_minus_2:
	lda	sound_freq_high,x
	beq	.borrow
.no_borrow
	dec	sound_freq_high,x
	rts
.borrow
	dec	sound_freq_high,x
	dec	sound_freq_n106,x
	rts
;-------------------------------------------------------------------------------
return62:
	indirect_lda	pitch_add_low	
	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_lp_table,y
	sta	pitch_add_low,x
	iny
	lda	pitchenve_lp_table,y
	sta	pitch_add_high,x
	jmp	pitch_sub
;-------------------------------------------------------------------------------
pitch_enverope_address:
	inc	pitch_add_low,x
	bne	return63
	inc	pitch_add_high,x
return63:
	rts
;-------------------------------------------------------------------------------
sound_high_speed_arpeggio:		;note enverope
;	lda	sound_freq_high,x
;	sta	temporary2
	jsr	note_enve_sub
	bcs	.end4			;0�Ȃ̂ŏ����Ȃ��Ă悵
	jsr	frequency_set
;.note_freq_write:
	jsr	reg400x
	lda	sound_freq_low,x
	sta	$4002,y
	lda	sound_freq_high,x
;	cmp	temporary2		;�����ɊԈ����,y�����Ă��̒����Ă݂����ǂ���ł��ς�����
;	cmp	$4003,y
;	beq	.end2
	sta	$4003,y
;.end2:
	jsr	arpeggio_address
	rts
.end4
;	jsr	frequency_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
note_add_set:
	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_lp_table,y
	sta	arpe_add_low,x
	iny
	lda	arpeggio_lp_table,y
	sta	arpe_add_high,x
	jmp	note_enve_sub
;-------------------------------------------------------------------------------
arpeggio_address:
	inc	arpe_add_low,x
	bne	return83
	inc	arpe_add_high,x
return83:
	rts
;-------------------------------------------------------------------------------
;----------------
;Output 
;	C=0(�ǂݍ��񂾒l��0����Ȃ��̂Ŕ�����������)
;	C=1(�ǂݍ��񂾒l��0�Ȃ̂Ŕ����������Ȃ��Ă�����)
;
note_enve_sub:

	jsr	reg400x
	indirect_lda	arpe_add_low		;�m�[�g�G���x�f�[�^�ǂݏo��
	cmp	#$ff			;$ff�i���I���j���H
	beq	note_add_set
	cmp	#$00			;�[�����H
	beq	.note_enve_zero_end	;�[���Ȃ�C���ĂĂ��I��
	cmp	#$80
	beq	.note_enve_zero_end	;�[���Ȃ�C���ĂĂ��I��
	jmp	.arpeggio_sign_check
.note_enve_zero_end
	sec				;���������͕s�v
	rts
.arpeggio_sign_check
	and	#%10000000
;	cmp	#%10000000		;�|���H
	bne	arpeggio_minus		;�|������

arpeggio_plus:
	indirect_lda	arpe_add_low		;�m�[�g�G���x�f�[�^��ǂݏo����
	sta	arpeggio_tmp		;�e���|�����ɒu���i���[�v�񐔁j
arpeggio_plus2:
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	and	#$0f			;����4bit���o
	cmp	#$0b			;����b�Ȃ�
	beq	oct_plus		;�I�N�^�[�u�{������
	inc	sound_sel,x		;�łȂ���Ή��K�{�P
	jmp	loop_1			;���[�v�����P��
oct_plus:
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	and	#$f0			;���4bit���o��������4bit�[��
	clc
	adc	#$10			;�I�N�^�[�u�{�P
	sta	sound_sel,x		;���K�f�[�^�����o��
loop_1:
	dec	arpeggio_tmp		;���[�v�񐔁|�P
	lda	arpeggio_tmp		;��œǂݏo��
;	cmp	#$00			;�[�����H
	beq	note_enve_end		;�Ȃ烋�[�v�����I���
	jmp	arpeggio_plus2		;�łȂ���΂܂�����

arpeggio_minus:
	indirect_lda	arpe_add_low	
	and	#%01111111
	sta	arpeggio_tmp
arpeggio_minus2:
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	and	#$0f			;����4bit���o
;	cmp	#$00			;�[�����H
	beq	oct_minus		;�[���Ȃ�|������
	dec	sound_sel,x		;�łȂ���Ή��K�|�P
	jmp	loop_2			;���[�v�����Q��
oct_minus:
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	clc
	adc	#$0b			;+b
	sec
	sbc	#$10			;-10
	sta	sound_sel,x		;���K�f�[�^�����o��
loop_2:
	dec	arpeggio_tmp		;���[�v�񐔁|�P
	lda	arpeggio_tmp		;��œǂݏo��
	cmp	#$00			;�[�����H
	bne	arpeggio_minus2		;�Ȃ烋�[�v�����I���
note_enve_end:
	clc				;���������͕K�v
	rts				;
;-------------------------------------------------------------------------------
;oto_set�ŌĂ΂��
effect_init:
;�\�t�g�E�F�A�G���x���[�v�ǂݍ��݃A�h���X������
	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_table,y
	sta	soft_add_low,x
	iny
	lda	softenve_table,y
	sta	soft_add_high,x

;�s�b�`�G���x���[�v�ǂݍ��݃A�h���X������
	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_table,y
	sta	pitch_add_low,x
	iny
	lda	pitchenve_table,y
	sta	pitch_add_high,x

;�f���[�e�B�G���x���[�v�ǂݍ��݃A�h���X������
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y
	sta	duty_add_low,x
	iny
	lda	dutyenve_table,y
	sta	duty_add_high,x

;�m�[�g�G���x���[�v�ǂݍ��݃A�h���X������
	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_table,y
	sta	arpe_add_low,x
	iny
	lda	arpeggio_table,y
	sta	arpe_add_high,x
;�\�t�g�E�F�ALFO������
	lda	lfo_count,x
	sta	lfo_start_count,x
	lda	lfo_adc_sbc_time,x
	sta	lfo_depth_count,x
	lda	lfo_harf_time,x
	sta	lfo_harf_count,x
	lda	lfo_revers,x
	sta	lfo_revers_count,x

	lda	channel_sel
	sec
	sbc	#$04
	bmi	urararara

	lda	effect_flag,x
	and	#%10111111
	ora	#%00100000
	sta	effect_flag,x
	jmp	ittoke
urararara:
	lda	effect_flag,x
	and	#%10011111
	sta	effect_flag,x
	
ittoke:
;�x���t���O�N���A&Key On�t���O��������
	lda	#%00000010
	sta	rest_flag,x
	rts


;-------------------------------------------------------------------------------
;internal 4 dpcm 1 fds 1 vrc7 6 vrc6 3 n106 8 fme7 3 mmc5 3(?)
;4+1+1+6+3+8+3+3=29ch
MAX_CH		=	$20		;32ch
;-------------------------------------------------------------------------------
;memory definition
;-------------------------------------------------------------------------------
;�[���y�[�W��������`
sound_add_low	=	$10		;command address
sound_add_high	=	$10+1		;

start_add_lsb	=	$f9
start_add_lsb_hi=	$fa
start_bank	=	$fe
start_bank_hi	=	$ff
reg_add_low	=	$f9	;temp
reg_add_high	=	$fa
x_escape	=	$fb
zero_add_low	=	$fc
zero_add_high	=	$fd
fds_wave_address=	$fe
fds_wave_addhigh=	$ff
vrc7_wave_add	=	$fe
vrc7_wave_add_hi=	$ff
VRC6_DST_REG_LOW=	$fe
VRC6_DST_REG_HIGH=	$ff
n106_wave_add	=	$fe
n106_wave_add_hi=	$ff
;����ȊO�ɕK�v�ȃ�����
;	jsr	reg400x
;	���Ă���
;	lda	memory,x
;	����̂�1�o�C�g�����Ƀf�[�^���Ȃ��

;_add_low��_add_high�͋ߐڂ��Ă���K�v������
soft_add_low	=	$0200+MAX_CH* 0  	;software envelope(@v) address
soft_add_high	=	$0200+MAX_CH* 0+1	;
pitch_add_low	=	$0200+MAX_CH* 2  	;pitch envelope (EP) address
pitch_add_high	=	$0200+MAX_CH* 2+1	;
duty_add_low	=	$0200+MAX_CH* 4  	;duty envelope (@@) address
duty_add_high	=	$0200+MAX_CH* 4+1	;
arpe_add_low	=	$0200+MAX_CH* 6  	;note envelope (EN) address
arpe_add_high	=	$0200+MAX_CH* 6+1	;
lfo_revers_count=	$0200+MAX_CH* 8  	;
lfo_depth_count	=	$0200+MAX_CH* 8+1	;
lfo_start_count	=	$0200+MAX_CH*10  	;
lfo_count	=	$0200+MAX_CH*10+1	;
lfo_adc_sbc_time=	$0200+MAX_CH*12  	;
lfo_depth	=	$0200+MAX_CH*12+1	;
lfo_revers	=	$0200+MAX_CH*14  	;
lfo_harf_time	=	$0200+MAX_CH*14+1	;���g�p
lfo_data873	=	$0200+MAX_CH*16  	;vibrato (MP) no
lfo_harf_count	=	$0200+MAX_CH*16+1	;?
detune_dat	=	$0200+MAX_CH*18  	;detune value
register_high	=	$0200+MAX_CH*18+1	;
register_low	=	$0200+MAX_CH*20  	;
duty_sel	=	$0200+MAX_CH*20+1	;duty envelope no
channel_loop	=	$0200+MAX_CH*22  	;|: :| loop counter
rest_flag	=	$0200+MAX_CH*22+1	;
softenve_sel	=	$0200+MAX_CH*24  	;software envelope(@v) no
pitch_sel	=	$0200+MAX_CH*24+1	;pitch envelope (EP) no
arpeggio_sel	=	$0200+MAX_CH*26  	;note envelope (EN) no
effect_flag	=	$0200+MAX_CH*26+1	;
sound_sel	=	$0200+MAX_CH*28  	;note no.
sound_counter	=	$0200+MAX_CH*28+1	;wait counter
sound_freq_low	=	$0200+MAX_CH*30  	;
sound_freq_high	=	$0200+MAX_CH*30+1	;
sound_freq_n106	=	$0200+MAX_CH*32  	;n106����Ȃ�ch�ł��g���Ă�
sound_bank	=	$0200+MAX_CH*32+1	;
extra_mem0	=	$0200+MAX_CH*34  	;����������ch���Ƃɗp�r���Ⴄ
extra_mem1	=	$0200+MAX_CH*34+1	;
extra_mem2	=	$0200+MAX_CH*36  	;

n106_volume	=	extra_mem0
n106_7c		=	extra_mem1

vrc7_key_stat	=	extra_mem0
vrc7_volume	=	extra_mem1

	.if	$0200+MAX_CH*38 > $0790
	.fail	"memory out of range"
	.endif
;���̑�
waru		=	$079c	;1byte
kotae		=	$079d	;1byte

detune_tmp	=	$07a1	;1byte
pitch_tmp	=	$07a3	;1byte
arpeggio_tmp	=	$07a5	;1byte
temporary	=	$07a7	;1byte
channel_sel	=	$07a9	;1byte
temporary2	=	$07ab

fds_hard_select	=	$07af
fds_volume	=	$07b1

n106_7f		=	$07b5

vrc7_temp	=	$07b8
n106_temp	=	$07b9
n106_temp_2	=	$07bb

fds_hard_count_1=	$07c0
fds_hard_count_2=	$07c1

initial_wait	=	$07c3

fme7_ch_sel	=	$07c5
fme7_ch_selx2	=	$07c6
fme7_data	=	$07c7
fme7_adr	=	$07c8


;effect_flag: DLLLadpv
;+------ detune flag
;l+----- software LFO�X�s�[�h�σt���O�i�\��j
;ll+---- software LFO�����t���O0=- 1=+
;lll+--- software LFO flag
;llll+---- note enverope flag
;lllll+--- duty enverope flag / FDS hardware effect flag
;llllll+-- pitch enverope flag
;lllllll+- software enverope flag
;llllllll
;DLLLadpv


;rest_flag
;xxxxxxor
;|||||||+- rest
;||||||+-- key on (if set, must do sound_data_write)
;
