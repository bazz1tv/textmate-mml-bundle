;FME-7
;C000 :AY 3-8910 address(W)
;E000 :data(W)
;
;Reference: http://www.howell1964.freeserve.co.uk/parts/ay3891x_datasheet.htm
;           http://www.breezer.demon.co.uk/spec/tech/ay-3-8912.html etc...
;
;AY 3-8910 Registers
;00	Ch. A freq data lower bits
;	FFFFFFFF
;01	Ch. A freq data higher bits
;	----FFFF
;	freq=1.79/(F*16) Mhz
;
;	
;02	Ch. B freq data lower bits
;03	Ch. B freq data higher bits
;04	Ch. C freq data lower bits
;05	Ch. C freq data higher bits
;
;
;06	Noise pitch
;	---FFFFF
;	freq=1.79/(F*16) Mhz
;
;
;07	Mixer
;	0:Enable 1:Disable
;	IINNNTTT
;	||||||||
;	|||||||+-------- Ch A Tone
;	||||||+--------- Ch B Tone
;	|||||+---------- Ch C Tone
;	||||+----------- Ch A Noise
;	|||+------------ Ch B Noise
;	||+------------- Ch C Noise
;	|+-------------- 
;	+--------------- 
;
;
;08	Ch. A volume
;	---MVVVV
;	V: Volume
;	M: Mode (1: Use envelope, 0:Use fixed volume)
;
;
;09	Ch. B volume
;0a	Ch. C volume
;
;
;0b	Envelope duration lower bits
;	DDDDDDDD
;0c	Envelope duration higher bits
;	DDDDDDDD
;	freq = 1.79/(D*256) Mhz
;	(duration = D*256/1.79 sec)
;
;
;0d	Envelope shape
;	----CAAH
;	    ||||
;	    |||+-------- Hold
;	    ||+--------- Alternate
;	    |+---------- Atack
;	    +----------- Continue
;
;
;0e	
;0f	

;Ch
;0b 0c 0d ���Z�b�g�Œ�`�ł���h���C�o�R�}���h�������������Ƃ���
;07

fme7w	.macro
	lda	\1
	sta	$c000
	lda	\2
	sta	$e000
	.endm

;-----------------------------------------------------------
fme7_sound_init:
	fme7w	#$07, #%11111000
	rts
;-----------------------------------------------------------
fme7_dst_adr_set:
	lda	channel_sel
	sec				
	sbc	#PTRFME7		;FME7�̉��`�����l���ڂ��H
	sta	fme7_ch_sel
	asl	a
	sta	fme7_ch_selx2
	rts

;-----------------------------------------------------------
;���W�X�^��������
fme7_ctrl_reg_write:
	lda	fme7_ch_sel
	clc
	adc	#$08
	sta	fme7_adr
	lda	channel_sel
	asl	a
	tax
	lda	register_low,x		;volume
	ora	register_high,x		;??
	and	#%00011111
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	rts

fme7_frq_reg_write:
	lda	fme7_ch_sel
	asl	a
	sta	fme7_adr
	lda	channel_sel
	asl	a
	tax
	lda	sound_freq_low,x
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	inc	fme7_adr
	lda	sound_freq_high,x
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	rts

fme7_frq_low_reg_write:
	lda	fme7_ch_sel
	asl	a
	sta	fme7_adr
	lda	channel_sel
	asl	a
	tax
	lda	sound_freq_low,x
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	rts

fme7_frq_high_reg_write:
	lda	fme7_ch_sel
	asl	a
	sta	fme7_adr
	inc	fme7_adr
	lda	channel_sel
	asl	a
	tax
	lda	sound_freq_high,x
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	rts

fme7_mute_write:
	lda	fme7_ch_sel
	clc
	adc	#$08
	sta	fme7_adr
	lda	channel_sel
	asl	a
	tax
	lda	register_low,x		;volume
	ora	register_high,x		;????
	and	#%00010000
	sta	fme7_data
	fme7w	fme7_adr, fme7_data
	rts

;---------------------------------------------------------------------------------------------------

sound_fme7:
	lda	channel_sel
	cmp	#PTRFME7+3
	beq	.end1
	jsr	fme7_dst_adr_set
	lda	channel_sel
	asl	a
	tax
	dec	sound_counter,x		;�J�E���^���������炵
	beq	.sound_read_go		;�[���Ȃ�T�E���h�ǂݍ���
	jsr	fme7_do_effect		;�[���ȊO�Ȃ�G�t�F�N�g����
	rts				;�����
.sound_read_go
	jsr	sound_fme7_read
	jsr	fme7_do_effect
	lda	rest_flag,x
	and	#%00000010		;�L�[�I���t���O
	beq	.end1			
	jsr	sound_fme7_write	;�����Ă�����f�[�^�����o��
	lda	rest_flag,x
	and	#%11111101		;�L�[�I���t���O�I�t
	sta	rest_flag,x
.end1
	rts

;-------
fme7_do_effect:
	lda	rest_flag,x
	and	#%00000001
	beq	.duty_write2
	rts				;�x���Ȃ�I���

.duty_write2:
	lda	effect_flag,x
	and	#%00000100
	beq	.enve_write2
	jsr	sound_fme7_dutyenve

.enve_write2:
	lda	effect_flag,x
	and	#%00000001
	beq	.lfo_write2
	jsr	sound_fme7_softenve

.lfo_write2:
	lda	effect_flag,x
	and	#%00010000
	beq	.pitchenve_write2
	jsr	sound_fme7_lfo

.pitchenve_write2:
	lda	effect_flag,x
	and	#%00000010
	beq	.arpeggio_write2
	jsr	sound_fme7_pitch_enve

.arpeggio_write2:
	lda	effect_flag,x
	and	#%00001000
	beq	.return7
	lda	rest_flag,x		;�L�[�I���̂Ƃ��Ƃ����łȂ��Ƃ��ŃA���y�W�I�̋����͂�����
	and	#%00000010		;�L�[�I���t���O
	bne	.arpe_key_on
	jsr	sound_fme7_note_enve	;�L�[�I������Ȃ��Ƃ��ʏ�͂���
	jmp	.return7
.arpe_key_on				;�L�[�I���������̏ꍇ
	jsr	note_enve_sub		;���������������ŁA�����ł͏������݂͂��Ȃ�
	jsr	fme7_freq_set
	jsr	arpeggio_address
.return7:
	rts
;------------------------------------------------
fme7_freq_set:
	lda	channel_sel
	asl	a
	tax
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	and	#%00001111		;����4bit�����o����
	asl	a
	tay

	lda	fme7_pls_frequency_table,y	;PSG���g���e�[�u������Low��ǂݏo��
	sta	sound_freq_low,x	;��������
	iny				;y���P���₵��
	lda	fme7_pls_frequency_table,y	;PSG���g���e�[�u������High��ǂݏo��
	sta	sound_freq_high,x	;��������

	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	lsr	a			;���4bit�����o��
	lsr	a			;
	lsr	a			;
	lsr	a			;
	beq	fme7_freq_end		;�[���Ȃ炻�̂܂܏I���
	tay

fme7_oct_set2:

	lsr	sound_freq_high,x	;�E�V�t�g�@������C��
	ror	sound_freq_low,x	;C���玝���Ă���ł�@�E���[�e�C�g
	dey				;
	bne	fme7_oct_set2		;�I�N�^�[�u���J��Ԃ�

fme7_freq_end:
	jsr	detune_write_sub
	rts
;---------------------------------------------------------------
sound_fme7_read:
	jsr	reg400x
	
	lda	sound_bank,x
	jsr	change_bank
	
	lda	[sound_add_low,x]
;----------
;���[�v����1
fme7_loop_program
	cmp	#$a0
	bne	fme7_loop_program2
	jsr	loop_sub
	jmp	sound_fme7_read
;----------
;���[�v����2(����)
fme7_loop_program2
	cmp	#$a1
	bne	fme7_bank_command
	jsr	loop_sub2
	jmp	sound_fme7_read
;----------
;�o���N�؂�ւ�
fme7_bank_command
	cmp	#$ee
	bne	fme7_wave_set
	jsr	data_bank_addr
	jmp	sound_fme7_read
;----------
;�f�[�^�G���h�ݒ�
;fme7_data_end:
;	cmp	#$ff
;	bne	fme7_wave_set
;	jsr	data_end_sub
;	jmp	sound_fme7_read
;----------
;���F�ݒ�
fme7_wave_set:
	cmp	#$fe
	bne	fme7_volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;���F�f�[�^�ǂݏo��
	pha
	and	#%10000000		;8bit�ڊm�F
	cmp	#%10000000
	bne	fme7_duty_enverope_part	;�a���[�e�B�G���x������

fme7_duty_select_part:
	lda	effect_flag,x
	and	#%11111011
	sta	effect_flag,x		;�f���[�e�B�G���x���[�v�����w��
	jsr	reg400x
	pla
	asl	a
	asl	a
	asl	a
	asl	a
	sta	register_high,x		;��������
	jsr	fme7_ctrl_reg_write
	jsr	sound_data_address
	jmp	sound_fme7_read

fme7_duty_enverope_part:
	lda	effect_flag,x
	ora	#%00000100
	sta	effect_flag,x		;�f���[�e�B�G���x���[�v�L���w��
	pla
	sta	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y	;�f���[�e�B�G���x���[�v�A�h���X�ݒ�
	sta	duty_add_low,x
	iny
	lda	dutyenve_table,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	sound_fme7_read

;----------
;���ʐݒ�
fme7_volume_set:
	cmp	#$fd
	bne	fme7_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	and	#%10000000		;bit7�m�F
	cmp	#%10000000
	bne	fme7_softenve_part		;�\�t�g�G���x������

fme7_volume_part:
	lda	effect_flag,x
	and	#%11111110
	sta	effect_flag,x		;�\�t�g�G���x�����w��

	lda	channel_sel
	lda	temporary
	and	#%00001111
	sta	register_low,x
	jsr	fme7_ctrl_reg_write
	jsr	sound_data_address
	jmp	sound_fme7_read

fme7_softenve_part:
	jsr	fme7_volume_sub
	jmp	sound_fme7_read
fme7_volume_sub:
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

;----------
fme7_rest_set:
	cmp	#$fc
	bne	fme7_lfo_set

	lda	rest_flag,x
	ora	#%00000001
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	jsr	fme7_mute_write

	jsr	sound_data_address
	rts
;----------
fme7_lfo_set:
	cmp	#$fb
	bne	fme7_detune_set
	jsr	lfo_set_sub
	jmp	sound_fme7_read
;----------
fme7_detune_set:
	cmp	#$fa
	bne	fme7_pitch_set
	jsr	detune_sub
	jmp	sound_fme7_read
;----------
;�s�b�`�G���x���[�v�ݒ�
fme7_pitch_set:
	cmp	#$f8
	bne	fme7_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_fme7_read
;----------
;�m�[�g�G���x���[�v�ݒ�
fme7_arpeggio_set:
	cmp	#$f7
	bne	fme7_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_fme7_read
;----------
;�Đ����g�����ڐݒ�
fme7_freq_direct_set:
	cmp	#$f6
	bne	fme7_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;���R�}���h�ݒ�
fme7_y_command_set:
	cmp	#$f5
	bne	fme7_wait_set
	jsr	y_sub
	jmp	sound_fme7_read
;----------
;�E�F�C�g�ݒ�
fme7_wait_set:
	cmp	#$f4
	bne	fme7_oto_set
	jsr	wait_sub
	rts
;----------
fme7_oto_set:
	sta	sound_sel,x		;�����͂܂����
	jsr	sound_data_address
	lda	[sound_add_low,x]	;�����ǂݏo��
	sta	sound_counter,x		;���ۂ̃J�E���g�l�ƂȂ�܂�
	jsr	sound_data_address
	jsr	fme7_freq_set		;���g���Z�b�g��
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
sound_fme7_write:
	jsr	fme7_ctrl_reg_write
	jsr	fme7_frq_reg_write
	rts
;-----------------------------------------------------
sound_fme7_softenve:
	jsr	volume_enve_sub
	sta	register_low,x
	jsr	fme7_ctrl_reg_write
	jsr	enverope_address	;�A�h���X����₵��
	rts				;�����܂�
;-------------------------------------------------------------------------------
sound_fme7_lfo:
	lda	sound_freq_high,x
	sta	temporary
	jsr	lfo_sub
	jsr	fme7_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	fme7_end4
	jsr	fme7_frq_high_reg_write
fme7_end4:
	rts
;-------------------------------------------------------------------------------
sound_fme7_pitch_enve:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
fme7_pitch_write:
	jsr	fme7_frq_low_reg_write
	lda	sound_freq_high,x
	cmp	temporary
	beq	fme7_end3
	jsr	fme7_frq_high_reg_write
fme7_end3:
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
sound_fme7_note_enve
;	lda	sound_freq_high,x
;	sta	temporary2
	jsr	note_enve_sub
	bcs	.end4			;0�Ȃ̂ŏ����Ȃ��Ă悵
	jsr	fme7_freq_set
;.fme7_note_freq_write:
	jsr	reg400x
	jsr	fme7_frq_low_reg_write
	lda	sound_freq_high,x
;	cmp	temporary2
;	beq	.fme7_end2
	jsr	fme7_frq_high_reg_write
;.fme7_end2:
	jsr	arpeggio_address
	rts
.end4
;	jsr	fme7_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
sound_fme7_dutyenve:
	jsr	reg400x

	indirect_lda	duty_add_low		;�G���x���[�v�f�[�^�ǂݍ���
	cmp	#$ff			;�Ōォ�ǁ[��
	beq	fme7_return22		;�Ō�Ȃ炻�̂܂܂����܂�
	asl	a
	asl	a
	asl	a
	asl	a
	sta	register_high,x
	jsr	fme7_ctrl_reg_write
	jsr	duty_enverope_address	;�A�h���X����₵��
	rts				;�����܂�

fme7_return22:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	iny
	lda	dutyenve_lp_table,y
	sta	duty_add_high,x
	jmp	sound_fme7_dutyenve
;-------------------------------------------------------------------------------
fme7_pls_frequency_table
;psg_frequency_table�̊e�l��2�{�Ɠ����͂��Ȃ̂�����
	dw	$0D5C, $0C9D, $0BE7, $0B3C
	dw	$0A9B, $0A02, $0973, $08EB
	dw	$086B, $07F2, $0780, $0714
	dw	$0000, $0FE4, $0EFF, $0E28

